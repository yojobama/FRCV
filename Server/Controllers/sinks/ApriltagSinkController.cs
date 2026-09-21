using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
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

        // --?-- PATCH: Apriltag Family Type;
    }
}
