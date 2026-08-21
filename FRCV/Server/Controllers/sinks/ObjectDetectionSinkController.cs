using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    internal class ObjectDetectionSinkController : WebApiController
    {
        // POST: create an object detection sink bound to a previously uploaded model
        [Route(HttpVerbs.Post, "/objectDetectionSink/create")]
        public Task<int> Create([QueryField] string name, [QueryField] int modelId)
        {
            int sinkId = SinkManager.Instance.AddObjectDetectionSink(name, modelId);
            return Task.FromResult(sinkId);
        }
    }
}
