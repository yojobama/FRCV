using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    internal class ObjectDetectionSinkController : WebApiController
    {
        // POST: create an object detection sink bound to a previously uploaded model. The
        // backend (ONNX Runtime vs RKNN/NPU) follows from the model itself (see
        // Model.Provider's own comment) - there is no separate provider parameter to pass here.
        [Route(HttpVerbs.Post, "/objectDetectionSink/create")]
        public Task<int> Create([QueryField] string name, [QueryField] int modelId)
        {
            int sinkId = SinkManager.Instance.AddObjectDetectionSink(name, modelId);
            return Task.FromResult(sinkId);
        }

        // GET: which backend an existing sink is actually running - informational only, see
        // ObjectDetectionSink::GetBackendName's own comment for why there's no PATCH to switch it.
        [Route(HttpVerbs.Get, "/objectDetectionSink/backend")]
        public Task<string> GetBackend([QueryField] int sinkId)
        {
            return Task.FromResult(SinkManager.Instance.GetObjectDetectionSinkBackendName(sinkId));
        }
    }
}
