using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    // ROADMAP.md Phase 8/E7: creation only - the actual stream itself is served by
    // MjpegStreamModule.cs (a raw multipart/x-mixed-replace HTTP response, not a normal REST
    // action, the same "custom EmbedIO module, not a controller" reasoning StateChannel.cs
    // already follows for /ws/state).
    internal class MjpegSinkController : WebApiController
    {
        // POST: create an MjpegSink. Bind it afterwards (PATCH /sink/bind) to the node whose
        // frames should be streamed, same as WebRTCSink.
        [Route(HttpVerbs.Post, "/mjpegSink/create")]
        public Task<int> Create([QueryField] string name, [QueryField] int jpegQuality = 80)
        {
            int sinkId = SinkManager.Instance.AddMjpegSink(name, jpegQuality);
            return Task.FromResult(sinkId);
        }
    }
}
