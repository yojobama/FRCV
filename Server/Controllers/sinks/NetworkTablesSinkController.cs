using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    internal class NetworkTablesSinkController : WebApiController
    {
        // POST: Create a NetworkTables sink that connects via team number
        [Route(HttpVerbs.Post, "/networkTablesSink/createForTeam")]
        public Task<int> CreateForTeam([QueryField] string name, [QueryField] int teamNumber,
            [QueryField] string rootTable = "lumenvision", [QueryField] string clientIdentity = "lumenvision")
        {
            int sinkId = SinkManager.Instance.AddNetworkTablesSinkForTeam(name, teamNumber, rootTable, clientIdentity);
            return Task.FromResult(sinkId);
        }

        // POST: Create a NetworkTables sink that connects to an explicit server address (bench testing)
        [Route(HttpVerbs.Post, "/networkTablesSink/createForServer")]
        public Task<int> CreateForServer([QueryField] string name, [QueryField] string serverAddress,
            [QueryField] int port = 0, [QueryField] string rootTable = "lumenvision", [QueryField] string clientIdentity = "lumenvision")
        {
            int sinkId = SinkManager.Instance.AddNetworkTablesSinkForServer(name, serverAddress, port, rootTable, clientIdentity);
            return Task.FromResult(sinkId);
        }

        // GET: NT4 connection status for a given sink
        [Route(HttpVerbs.Get, "/networkTablesSink/status")]
        public Task<NetworkTablesStatusDto> GetStatus([QueryField] int sinkId)
        {
            return Task.FromResult(NetworkTablesStatusDto.Parse(SinkManager.Instance.GetNetworkTablesSinkStatus(sinkId)));
        }
    }
}
