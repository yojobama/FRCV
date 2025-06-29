using Microsoft.AspNetCore.SignalR;

namespace FRCVHost.Services
{
    public class VisionHub : Hub
    {
        private readonly IWebRTCService _webRTCService;

        public VisionHub(IWebRTCService webRTCService)
        {
            _webRTCService = webRTCService;
        }

        public async Task JoinSession(string sessionId)
        {
            await Groups.AddToGroupAsync(Context.ConnectionId, sessionId);
        }

        public async Task SendOffer(string sessionId, object offer)
        {
            await Clients.Group(sessionId).SendAsync("ReceiveOffer", offer);
        }

        public async Task SendAnswer(string sessionId, object answer)
        {
            await Clients.Group(sessionId).SendAsync("ReceiveAnswer", answer);
        }

        public async Task SendIceCandidate(string sessionId, object candidate)
        {
            await Clients.Group(sessionId).SendAsync("ReceiveIceCandidate", candidate);
        }

        public override async Task OnDisconnectedAsync(Exception exception)
        {
            // Cleanup any active sessions
            await base.OnDisconnectedAsync(exception);
        }
    }
}