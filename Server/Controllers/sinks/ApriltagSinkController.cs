using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    internal class ApriltagSinkController : WebApiController
    {
        // POST: Create an Apriltag sink
        [Route(HttpVerbs.Post, "/apriltagSink/create")]
        public Task<int> Create([QueryField] string name, [QueryField] string type)
        {
            int SinkID = SinkManager.Instance.AddSink(name, type);
            DB.Instance.Save();
            return Task.FromResult(SinkID);
        }

        // POST: Create an Apriltag sink that reuses the calibration result of an existing CameraCalibrationSink,
        // transferring the calibration data so the detected tag's real world location can be computed
        [Route(HttpVerbs.Post, "/apriltagSink/createFromCalibrator")]
        public Task<int> CreateFromCalibrator([QueryField] string name, [QueryField] int calibratorId, [QueryField] double tagSize)
        {
            int sinkId = SinkManager.Instance.AddApriltagSinkFromCalibrator(name, calibratorId, tagSize);
            return Task.FromResult(sinkId);
        }

        // POST: Create an Apriltag sink with an explicit backend (cpu/vulkan) and no calibration
        // data yet; frameWidth/frameHeight only matter for the Vulkan backend
        [Route(HttpVerbs.Post, "/apriltagSink/createWithBackend")]
        public Task<int> CreateWithBackend([QueryField] string name, [QueryField] double tagSize,
            [QueryField] ApriltagBackendKind backend, [QueryField] int frameWidth = 0, [QueryField] int frameHeight = 0)
        {
            int sinkId = SinkManager.Instance.AddApriltagSinkWithBackend(name, tagSize, backend, frameWidth, frameHeight);
            return Task.FromResult(sinkId);
        }

        // GET: which backend a sink actually ended up running (may differ from what was
        // requested - Vulkan falls back to CPU if no usable device was found)
        [Route(HttpVerbs.Get, "/apriltagSink/backend")]
        public Task<string> GetBackend([QueryField] int sinkId)
        {
            return Task.FromResult(SinkManager.Instance.GetApriltagBackendName(sinkId));
        }

        // GET: the same thing as /backend, as the real enum rather than a display string - lets
        // the webui's Inspector pre-select the sink's actual current backend in its dropdown.
        [Route(HttpVerbs.Get, "/apriltagSink/backendKind")]
        public Task<ApriltagBackendKind> GetBackendKind([QueryField] int sinkId)
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
        [Route(HttpVerbs.Get, "/apriltagSink/tuning")]
        public Task<ApriltagTuningDto> GetTuning([QueryField] int sinkId)
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
        [Route(HttpVerbs.Patch, "/apriltagSink/backend")]
        public Task SetBackend([QueryField] int sinkId, [QueryField] ApriltagBackendKind backend,
            [QueryField] int? nthreads = null, [QueryField] float? quadDecimate = null)
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
        [Route(HttpVerbs.Post, "/apriltagSink/fieldLayout")]
        public async Task<int> SetFieldLayout([QueryField] int sinkId)
        {
            using var reader = new StreamReader(HttpContext.OpenRequestStream());
            string json = await reader.ReadToEndAsync();

            string layoutDir = Path.Combine(AppContext.BaseDirectory, "fieldLayouts");
            Directory.CreateDirectory(layoutDir);
            string path = Path.Combine(layoutDir, $"sink-{sinkId}.json");
            await File.WriteAllTextAsync(path, json);

            bool ok = ManagerWrapper.Instance.LoadFieldLayout(sinkId, path);
            return ok ? ManagerWrapper.Instance.GetFieldLayoutTagCount(sinkId) : -1;
        }

        // GET: how many tags this sink's currently-loaded field layout has (0 if none loaded)
        [Route(HttpVerbs.Get, "/apriltagSink/fieldLayoutTagCount")]
        public Task<int> GetFieldLayoutTagCount([QueryField] int sinkId)
        {
            return Task.FromResult(ManagerWrapper.Instance.GetFieldLayoutTagCount(sinkId));
        }
    }
}
