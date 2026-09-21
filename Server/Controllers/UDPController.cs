using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers
{
    internal class UDPController : WebApiController
    {
        private List<UDPTransmiter> udpTransmiters = new List<UDPTransmiter>();

        // POST: Start udp transmittions to the client address on a specified port;
        [Route(HttpVerbs.Post, "/udp/start")]
        public Task StartUDPTransmition()
        {
            SinkManager.Instance.EnableManagerThread();
            var transmitter = new UDPTransmiter(SinkManager.Instance.createResultChannel(), HttpContext.Request.RemoteEndPoint.Address.ToString(), 12345);
            transmitter.Enable();
            udpTransmiters.Add(transmitter);
            return Task.CompletedTask;
        }
        // POST: Stop udp transmittions to the client address;
        [Route(HttpVerbs.Post, "/udp/stop")]
        public Task StopUDPTransmition()
        {
            string clientAddress = HttpContext.Request.RemoteEndPoint.Address.ToString();
            var transmitter = udpTransmiters.FirstOrDefault(x => x.HostName == clientAddress);
            if (transmitter != null)
            {
                transmitter.Disable();
                udpTransmiters.Remove(transmitter);
            }
            if (udpTransmiters.Count == 0)
            {
                SinkManager.Instance.DisableManagerThread();
            }
            return Task.CompletedTask;
        }
    }
}
