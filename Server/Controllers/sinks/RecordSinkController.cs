using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sinks
{
    public class RecordSegmentDto
    {
        public string FileName { get; set; } = "";
        public long SizeBytes { get; set; }
        public DateTime LastWriteTimeUtc { get; set; }
    }

    // Recording, for both post-match telemetry and footage teams download to feature in videos -
    // see RecordSink.h's own comment for the design (segmented MP4 + a JSON-Lines sidecar per
    // segment). This is also the first controller in this project to serve a file back over HTTP
    // at all - no generic download/static-file endpoint existed anywhere before this.
    internal class RecordSinkController : ControllerBase
    {
        // POST: create a RecordSink. Bind it afterwards (PATCH /sink/bind) to the node whose
        // frames should be recorded, same as WebRTCSink/MjpegSink. dstFolder defaults to null
        // (resolved from the sink's own name - see SinkManager.AddRecordSink's own comment) so a
        // caller doesn't have to invent a path just to start recording.
        //
        // Every numeric parameter is nullable here, resolved to its real default in the method
        // body via ?? - NOT a plain `int x = 8000`-style C# default parameter. Confirmed the hard
        // way: EmbedIO's [QueryField] binding does not apply a value-type parameter's own C#
        // default when the query string omits that key - it silently binds default(int) (0)
        // instead, which happened to make fps and bitrateKbps 0 and avcodec_open2 fail outright
        // ("The encoder timebase is not set") the first time this endpoint was called without
        // every parameter spelled out. A nullable value type's "absent" state is unambiguous, so
        // the binder leaves it null instead of guessing - the same reason dstFolder/encoderName
        // above were already nullable strings rather than defaulted non-nullable ones.
        [HttpPost("recordSink/create")]
        public Task<int> Create([FromQuery] string name, [FromQuery] string? dstFolder = null,
            [FromQuery] string? encoderName = null, [FromQuery] int? bitrateKbps = null,
            [FromQuery] int? fps = null, [FromQuery] int? segmentSeconds = null,
            [FromQuery] long? maxFolderSizeBytes = null, [FromQuery] int? maxFileCount = null)
        {
            int sinkId = SinkManager.Instance.AddRecordSink(name, dstFolder, encoderName,
                bitrateKbps ?? 8000, fps ?? 30, segmentSeconds ?? 300, maxFolderSizeBytes ?? 0, maxFileCount ?? 0);
            return Task.FromResult(sinkId);
        }

        // GET: every recorded segment for this sink, newest first, with size/last-write-time -
        // real video duration isn't probed here (would need an actual container parse, not just
        // a filesystem stat); a segment's own JSON-Lines sidecar carries real per-frame
        // timestamps for anything that needs precise timing.
        [HttpGet("recordSink/{id}/segments")]
        public Task<List<RecordSegmentDto>> GetSegments(int id)
        {
            var sink = RequireRecordSink(id);
            var result = new List<RecordSegmentDto>();
            foreach (var filename in SinkManager.Instance.GetRecordSinkSegments(id))
            {
                if (!TryResolveSegmentPath(sink.RecordDstFolder!, filename, out string fullPath)) continue;
                var info = new FileInfo(fullPath);
                if (!info.Exists) continue;
                result.Add(new RecordSegmentDto { FileName = filename, SizeBytes = info.Length, LastWriteTimeUtc = info.LastWriteTimeUtc });
            }
            return Task.FromResult(result);
        }

        // GET: download one segment (the video itself, or its .jsonl telemetry sidecar) as a raw
        // byte stream - genuinely new ground for this project, see this file's own top comment.
        // `file` is resolved strictly against this sink's OWN RecordDstFolder (TryResolveSegmentPath),
        // never trusted as a caller-supplied path directly - the obvious trap for a brand-new
        // "serve a file by name" endpoint.
        //
        // PhysicalFile with range processing: a browser <video> element (or a download manager)
        // can seek/resume via HTTP Range requests instead of re-fetching a multi-hundred-MB match
        // recording from the start. fileDownloadName sets the same attachment Content-Disposition
        // the old hand-written header did.
        [HttpGet("recordSink/{id}/download")]
        public Task<IActionResult> Download(int id, [FromQuery] string file)
        {
            var sink = RequireRecordSink(id);
            if (!TryResolveSegmentPath(sink.RecordDstFolder!, file, out string fullPath) || !System.IO.File.Exists(fullPath))
            {
                throw ApiException.NotFound();
            }
            string contentType = Path.GetExtension(fullPath) == ".jsonl" ? "application/x-ndjson" : "video/mp4";
            IActionResult result = PhysicalFile(Path.GetFullPath(fullPath), contentType, Path.GetFileName(fullPath), enableRangeProcessing: true);
            return Task.FromResult(result);
        }

        // POST: use an already-recorded segment as a VideoFileSource directly, no re-upload - the
        // old (deleted) RecordSink stub's own second aspirational comment, now real. Reuses
        // VideoFileSourceController's exact underlying call (InitializeVideoFileSource) against
        // the file already on disk.
        // POST: start (enabled=true) or stop (enabled=false) recording on EVERY source at once -
        // the Match View button. The robot does the same over NT (<root>/config/recording); both
        // go through SinkManager.SetAllRecording. Returns how many RecordSinks are now running.
        [HttpPost("recordSink/all")]
        public Task<int> SetAllRecording([FromQuery] bool enabled)
        {
            return Task.FromResult(SinkManager.Instance.SetAllRecording(enabled));
        }

        // GET: whether any RecordSink is currently running
        [HttpGet("recordSink/all")]
        public Task<bool> IsAnyRecording()
        {
            return Task.FromResult(SinkManager.Instance.IsAnyRecording());
        }

        [HttpPost("recordSink/{id}/promote")]
        public Task<int> Promote(int id, [FromQuery] string file, [FromQuery] string? name = null)
        {
            var sink = RequireRecordSink(id);
            if (!TryResolveSegmentPath(sink.RecordDstFolder!, file, out string fullPath) || !System.IO.File.Exists(fullPath))
            {
                throw ApiException.NotFound();
            }
            int sourceId = SourceManager.Instance.InitializeVideoFileSource(fullPath, 30, name ?? Path.GetFileNameWithoutExtension(file));
            return Task.FromResult(sourceId);
        }

        // DELETE: remove one segment (and its .jsonl sidecar) manually, alongside the automatic
        // retention EnforceRetention() already applies.
        [HttpDelete("recordSink/{id}/segments")]
        public Task<bool> DeleteSegment(int id, [FromQuery] string file)
        {
            RequireRecordSink(id);
            return Task.FromResult(SinkManager.Instance.DeleteRecordSinkSegment(id, file));
        }

        private static Sink RequireRecordSink(int id)
        {
            var sink = SinkManager.Instance.GetSinkById(id);
            if (sink == null || sink.Type != SinkType.RecordSink || string.IsNullOrEmpty(sink.RecordDstFolder))
            {
                throw ApiException.NotFound();
            }
            return sink;
        }

        // filename must be a bare name (no path separators/"..") AND resolve to somewhere inside
        // dstFolder once normalized - the two checks together are what actually close the
        // path-traversal trap; either alone can be bypassed (a bare-looking name can still
        // resolve outside on some platforms, and a "no .." check alone doesn't catch every
        // absolute-path form).
        private static bool TryResolveSegmentPath(string dstFolder, string filename, out string fullPath)
        {
            fullPath = "";
            if (string.IsNullOrEmpty(filename) || filename.Contains("..") || filename.Contains('/') || filename.Contains('\\'))
            {
                return false;
            }
            string folderFull = Path.GetFullPath(dstFolder);
            string candidate = Path.GetFullPath(Path.Combine(folderFull, filename));
            if (!candidate.StartsWith(folderFull + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
            {
                return false;
            }
            fullPath = candidate;
            return true;
        }
    }
}
