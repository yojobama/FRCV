using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

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
    internal class RecordSinkController : WebApiController
    {
        // POST: create a RecordSink. Bind it afterwards (PATCH /sink/bind) to the node whose
        // frames should be recorded, same as WebRTCSink/MjpegSink. dstFolder defaults to null
        // (resolved from the sink's own name - see SinkManager.AddRecordSink's own comment) so a
        // caller doesn't have to invent a path just to start recording.
        [Route(HttpVerbs.Post, "/recordSink/create")]
        public Task<int> Create([QueryField] string name, [QueryField] string? dstFolder = null,
            [QueryField] string? encoderName = null, [QueryField] int bitrateKbps = 8000,
            [QueryField] int fps = 30, [QueryField] int segmentSeconds = 300,
            [QueryField] long maxFolderSizeBytes = 0, [QueryField] int maxFileCount = 0)
        {
            int sinkId = SinkManager.Instance.AddRecordSink(name, dstFolder, encoderName, bitrateKbps, fps, segmentSeconds, maxFolderSizeBytes, maxFileCount);
            return Task.FromResult(sinkId);
        }

        // GET: every recorded segment for this sink, newest first, with size/last-write-time -
        // real video duration isn't probed here (would need an actual container parse, not just
        // a filesystem stat); a segment's own JSON-Lines sidecar carries real per-frame
        // timestamps for anything that needs precise timing.
        [Route(HttpVerbs.Get, "/recordSink/{id}/segments")]
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
        [Route(HttpVerbs.Get, "/recordSink/{id}/download")]
        public async Task Download(int id, [QueryField] string file)
        {
            var sink = RequireRecordSink(id);
            if (!TryResolveSegmentPath(sink.RecordDstFolder!, file, out string fullPath) || !File.Exists(fullPath))
            {
                throw HttpException.NotFound();
            }
            HttpContext.Response.ContentType = Path.GetExtension(fullPath) == ".jsonl" ? "application/x-ndjson" : "video/mp4";
            HttpContext.Response.Headers.Add("Content-Disposition", $"attachment; filename=\"{Path.GetFileName(fullPath)}\"");
            using var fileStream = File.OpenRead(fullPath);
            await fileStream.CopyToAsync(HttpContext.Response.OutputStream, HttpContext.CancellationToken);
        }

        // POST: use an already-recorded segment as a VideoFileSource directly, no re-upload - the
        // old (deleted) RecordSink stub's own second aspirational comment, now real. Reuses
        // VideoFileSourceController's exact underlying call (InitializeVideoFileSource) against
        // the file already on disk.
        [Route(HttpVerbs.Post, "/recordSink/{id}/promote")]
        public Task<int> Promote(int id, [QueryField] string file, [QueryField] string? name = null)
        {
            var sink = RequireRecordSink(id);
            if (!TryResolveSegmentPath(sink.RecordDstFolder!, file, out string fullPath) || !File.Exists(fullPath))
            {
                throw HttpException.NotFound();
            }
            int sourceId = SourceManager.Instance.InitializeVideoFileSource(fullPath, 30, name ?? Path.GetFileNameWithoutExtension(file));
            return Task.FromResult(sourceId);
        }

        // DELETE: remove one segment (and its .jsonl sidecar) manually, alongside the automatic
        // retention EnforceRetention() already applies.
        [Route(HttpVerbs.Delete, "/recordSink/{id}/segments")]
        public Task<bool> DeleteSegment(int id, [QueryField] string file)
        {
            RequireRecordSink(id);
            return Task.FromResult(SinkManager.Instance.DeleteRecordSinkSegment(id, file));
        }

        private static Sink RequireRecordSink(int id)
        {
            var sink = SinkManager.Instance.GetSinkById(id);
            if (sink == null || sink.Type != SinkType.RecordSink || string.IsNullOrEmpty(sink.RecordDstFolder))
            {
                throw HttpException.NotFound();
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
