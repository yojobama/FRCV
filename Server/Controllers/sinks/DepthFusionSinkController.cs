using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    // Fuses a detector's bounding boxes with a StereoDepthSink's depth grid - see
    // STEREO_IMPLEMENTATION_PLAN.md ss10.4. Bind the detector with the ordinary
    // PATCH /api/sink/bind (it must itself be bound to the StereoDepthSink's own rectified-left
    // frame output, not a raw camera); attach the depth source separately via /attachDepthSource.
    internal class DepthFusionSinkController : WebApiController
    {
        // POST: create a DepthFusionSink
        [Route(HttpVerbs.Post, "/depthFusionSink/create")]
        public Task<int> Create([QueryField] string name)
        {
            int sinkId = SinkManager.Instance.AddDepthFusionSink(name);
            return Task.FromResult(sinkId);
        }

        // PATCH: attach the StereoDepthSink this node reads its depth grid from directly - not
        // the same as binding a source (see DepthFusionNode.h).
        [Route(HttpVerbs.Patch, "/depthFusionSink/{id}/attachDepthSource")]
        public Task AttachDepthSource(int id, [QueryField] int stereoDepthSinkId)
        {
            SinkManager.Instance.AttachDepthFusionSource(id, stereoDepthSinkId);
            return Task.CompletedTask;
        }
    }
}
