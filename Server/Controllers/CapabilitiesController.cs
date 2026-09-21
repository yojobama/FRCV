using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Linq;
using System.Threading.Tasks;

namespace Server.Controllers
{
    // ROADMAP.md Phase 8a: exposes what this build can actually do, so the webui can grey out
    // unavailable options and validate the graph structurally instead of discovering both only
    // from a failed request. GetEnabledFeatures() (ROADMAP.md Phase 2d) has existed natively
    // since the CMake migration but had zero REST exposure until now.
    internal class CapabilitiesController : WebApiController
    {
        // GET: which LUMEN_WITH_* features this build was actually compiled with (e.g. "ONNX",
        // "NT4", "WEBRTC", "VULKAN_APRILTAG", "CODEC_STEREO", "RKNN") - a method whose body is
        // #ifdef'd out throws a clear runtime error today; this is what lets the UI avoid
        // offering it in the first place.
        [Route(HttpVerbs.Get, "/capabilities/features")]
        public Task<string[]> GetEnabledFeatures()
        {
            return Task.FromResult(ManagerWrapper.Instance.GetEnabledFeatures().ToArray());
        }

        // GET: every source/sink node type this webui can create, with its wiring rules
        // (source count, role labels, dual-role/depth-attach behaviour) - see NodeCapabilities.cs.
        [Route(HttpVerbs.Get, "/capabilities/nodeTypes")]
        public Task<NodeTypesResponse> GetNodeTypes()
        {
            return Task.FromResult(new NodeTypesResponse(NodeCapabilities.Sources.ToArray(), NodeCapabilities.Sinks.ToArray()));
        }
    }

    public record struct NodeTypesResponse(NodeTypeCapability[] Sources, NodeTypeCapability[] Sinks);
}
