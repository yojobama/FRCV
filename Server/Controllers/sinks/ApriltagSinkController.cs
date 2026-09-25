using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sinks
{
    internal class ApriltagSinkController : ControllerBase
    {
        // POST: Create an Apriltag sink
        [HttpPost("apriltagSink/create")]
        public Task<int> Create([FromQuery] string name, [FromQuery] string type)
        {
            int SinkID = SinkManager.Instance.AddSink(name, type);
            DB.Instance.Save();
            return Task.FromResult(SinkID);
        }

        // POST: Create an Apriltag sink that reuses the calibration result of an existing CameraCalibrationSink,
        // transferring the calibration data so the detected tag's real world location can be computed
        [HttpPost("apriltagSink/createFromCalibrator")]
        public Task<int> CreateFromCalibrator([FromQuery] string name, [FromQuery] int calibratorId, [FromQuery] double tagSize)
        {
            int sinkId = SinkManager.Instance.AddApriltagSinkFromCalibrator(name, calibratorId, tagSize);
            return Task.FromResult(sinkId);
        }

        // POST: Create an Apriltag sink with an explicit backend (cpu/vulkan) and no calibration
        // data yet; frameWidth/frameHeight only matter for the Vulkan backend
        [HttpPost("apriltagSink/createWithBackend")]
        public Task<int> CreateWithBackend([FromQuery] string name, [FromQuery] double tagSize,
            [FromQuery] ApriltagBackendKind backend, [FromQuery] int frameWidth = 0, [FromQuery] int frameHeight = 0)
        {
            int sinkId = SinkManager.Instance.AddApriltagSinkWithBackend(name, tagSize, backend, frameWidth, frameHeight);
            return Task.FromResult(sinkId);
        }

        // GET: which backend a sink actually ended up running (may differ from what was
        // requested - Vulkan falls back to CPU if no usable device was found)
        [HttpGet("apriltagSink/backend")]
        public Task<string> GetBackend([FromQuery] int sinkId)
        {
            return Task.FromResult(SinkManager.Instance.GetApriltagBackendName(sinkId));
        }

        // GET: the same thing as /backend, as the real enum rather than a display string - lets
        // the webui's Inspector pre-select the sink's actual current backend in its dropdown.
        [HttpGet("apriltagSink/backendKind")]
        public Task<ApriltagBackendKind> GetBackendKind([FromQuery] int sinkId)
        {
            return Task.FromResult(ManagerWrapper.Instance.GetApriltagDetectorBackendKind(sinkId));
        }

        // GET: the sink's current tuning - threads and quad_decimate are genuinely
        // user-adjustable (not hardcoded, see ApriltagDetector's own constructor comment), so the
        // Inspector needs this to pre-populate its Threads/QuadDecimate controls the same way
        // /backendKind pre-populates the Backend dropdown. QuadDecimateSupported is false for
        // Vulkan (fixed 2x decimation baked into its compute pipeline - see
        // VkApriltagBackend::GetQuadDecimate's own comment) - the webui hides/disables the
        // QuadDecimate control when this is false rather than letting a user set a value that's
        // silently ignored.
        [HttpGet("apriltagSink/tuning")]
        public Task<ApriltagTuningDto> GetTuning([FromQuery] int sinkId)
        {
            return Task.FromResult(new ApriltagTuningDto
            {
                Threads = ManagerWrapper.Instance.GetApriltagDetectorThreads(sinkId),
                QuadDecimate = ManagerWrapper.Instance.GetApriltagDetectorQuadDecimate(sinkId),
                QuadDecimateSupported = ManagerWrapper.Instance.GetApriltagDetectorQuadDecimateSupported(sinkId),
            });
        }

        // PATCH: switches an EXISTING sink between CPU/Vulkan in place, preserving its id, tag
        // size, calibration, driver mode, and every binding (upstream camera + any downstream
        // WebRTC/NT4 sinks) - see SinkManager.SetApriltagBackend's own comment for why this has
        // to tear down and recreate the detector rather than mutating it. This is the sink's own
        // "Backend" control, not the Pipeline Profiles one (PipelineProfileController) - that one
        // only helps if a profile was set up in advance; this works on any plain ApriltagSink.
        // nthreads/quadDecimate are optional - when omitted, SetApriltagBackend carries forward
        // the sink's current tuning rather than resetting it, so a plain backend switch doesn't
        // silently clobber tuning the user already dialled in.
        [HttpPatch("apriltagSink/backend")]
        public Task SetBackend([FromQuery] int sinkId, [FromQuery] ApriltagBackendKind backend,
            [FromQuery] int? nthreads = null, [FromQuery] float? quadDecimate = null)
        {
            SinkManager.Instance.SetApriltagBackend(sinkId, backend, nthreads, quadDecimate);
            return Task.CompletedTask;
        }

        // --?-- PATCH: Apriltag Family Type;

        // POST: upload a WPILib-format AprilTagFieldLayout JSON body (the same file a robot
        // program's own WPILib code already loads) and enable multi-tag PnP on this sink -
        // ROADMAP.md Phase 7. Written to a fixed directory (not a caller-supplied path), same
        // reasoning as /source/snapshot. Returns the number of tags actually loaded, or -1 if
        // the body wasn't a valid field layout.
        [HttpPost("apriltagSink/fieldLayout")]
        public async Task<int> SetFieldLayout([FromQuery] int sinkId)
        {
            using var reader = new StreamReader(HttpContext.OpenRequestStream());
            string json = await reader.ReadToEndAsync();

            string layoutDir = Path.Combine(AppContext.BaseDirectory, "fieldLayouts");
            Directory.CreateDirectory(layoutDir);
            string path = Path.Combine(layoutDir, $"sink-{sinkId}.json");
            await System.IO.File.WriteAllTextAsync(path, json);

            bool ok = ManagerWrapper.Instance.LoadFieldLayout(sinkId, path);
            return ok ? ManagerWrapper.Instance.GetFieldLayoutTagCount(sinkId) : -1;
        }

        // GET: how many tags this sink's currently-loaded field layout has (0 if none loaded)
        [HttpGet("apriltagSink/fieldLayoutTagCount")]
        public Task<int> GetFieldLayoutTagCount([FromQuery] int sinkId)
        {
            return Task.FromResult(ManagerWrapper.Instance.GetFieldLayoutTagCount(sinkId));
        }
    }
}
