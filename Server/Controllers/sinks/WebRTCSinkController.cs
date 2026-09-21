using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    // REST-based WebRTC signalling. WebRTCSink uses non-trickle ICE on LumenVision's side (it blocks
    // internally until its own candidate gathering completes before returning an offer), so no
    // persistent connection is needed here beyond each request's own lifetime; the browser's own
    // candidates (which most browsers still trickle one at a time) are added as they arrive via
    // repeated calls to /webrtcSink/candidate.
    internal class WebRTCSinkController : WebApiController
    {
        // POST: create a WebRTCSink. Bind it afterwards (PATCH /sink/bind) to the node whose
        // frames should be streamed.
        [Route(HttpVerbs.Post, "/webrtcSink/create")]
        public Task<int> Create([QueryField] string name, [QueryField] int bitrateKbps = 4000,
            [QueryField] int fps = 30, [QueryField] string encoderName = "libx264")
        {
            int sinkId = SinkManager.Instance.AddWebRTCSink(name, bitrateKbps, fps, encoderName);
            return Task.FromResult(sinkId);
        }

        // POST: get an SDP offer from a WebRTCSink (blocks briefly for ICE gathering). Written
        // as a raw text/plain body rather than returned as Task<string> - EmbedIO's default
        // response serializer for a plain string return value does not escape control
        // characters, and SDP is full of literal \r\n line endings, so JSON-wrapping it that
        // way produces a syntactically invalid JSON string (confirmed the hard way: the browser
        // got "Bad control character in string literal in JSON" trying to parse it).
        [Route(HttpVerbs.Post, "/webrtcSink/offer")]
        public async Task CreateOffer([QueryField] int sinkId)
        {
            string sdp = SinkManager.Instance.WebRTCCreateOffer(sinkId);
            await HttpContext.SendStringAsync(sdp, "text/plain", Encoding.UTF8);
        }

        // POST: submit the browser's SDP answer
        [Route(HttpVerbs.Post, "/webrtcSink/answer")]
        public async Task SetAnswer([QueryField] int sinkId)
        {
            using var reader = new StreamReader(HttpContext.OpenRequestStream());
            string sdp = await reader.ReadToEndAsync();
            SinkManager.Instance.WebRTCSetAnswer(sinkId, sdp);
        }

        // POST: submit one of the browser's trickled ICE candidates
        [Route(HttpVerbs.Post, "/webrtcSink/candidate")]
        public Task AddIceCandidate([QueryField] int sinkId, [QueryField] string candidate, [QueryField] string mid)
        {
            SinkManager.Instance.WebRTCAddIceCandidate(sinkId, candidate, mid);
            return Task.CompletedTask;
        }

        // GET: connection status ({"connected":bool,"iceState":...,"gatheringComplete":bool})
        [Route(HttpVerbs.Get, "/webrtcSink/status")]
        public Task<string> GetStatus([QueryField] int sinkId)
        {
            return Task.FromResult(SinkManager.Instance.GetWebRTCSinkStatus(sinkId));
        }
    }
}
