using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sinks
{
    // Fuses a detector's bounding boxes with a StereoDepthSink's depth grid - see
    // STEREO_IMPLEMENTATION_PLAN.md ss10.4. Bind the detector with the ordinary
    // PATCH /api/sink/bind (it must itself be bound to the StereoDepthSink's own rectified-left
    // frame output, not a raw camera); attach the depth source separately via /attachDepthSource.
    internal class DepthFusionSinkController : ControllerBase
    {
        // POST: create a DepthFusionSink
        [HttpPost("depthFusionSink/create")]
        public Task<int> Create([FromQuery] string name)
        {
            int sinkId = SinkManager.Instance.AddDepthFusionSink(name);
            return Task.FromResult(sinkId);
        }

        // PATCH: attach the StereoDepthSink this node reads its depth grid from directly - not
        // the same as binding a source (see DepthFusionNode.h).
        [HttpPatch("depthFusionSink/{id}/attachDepthSource")]
        public Task AttachDepthSource(int id, [FromQuery] int stereoDepthSinkId)
        {
            SinkManager.Instance.AttachDepthFusionSource(id, stereoDepthSinkId);
            return Task.CompletedTask;
        }
    }
}
