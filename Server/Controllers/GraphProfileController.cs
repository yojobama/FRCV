using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Server.Controllers
{
    // ROADMAP.md Phase 8/E6: save/restore the whole node graph under a name - see
    // GraphProfile.cs's own comment for how this differs from the existing per-source
    // PipelineProfile.
    internal class GraphProfileController : WebApiController
    {
        // GET: every saved whole-graph profile name, for the top-bar dropdown / LeftRail list.
        [Route(HttpVerbs.Get, "/graphProfile/list")]
        public Task<List<string>> List()
        {
            return Task.FromResult(GraphProfile.Instance.ListProfiles());
        }

        // POST: snapshot the CURRENTLY LIVE graph (every source, sink, and binding) under `name`
        // - overwrites any existing profile with the same name.
        [Route(HttpVerbs.Post, "/graphProfile/saveCurrentAs")]
        public Task SaveCurrentAs([QueryField] string name)
        {
            GraphProfile.Instance.SaveCurrentAs(name);
            return Task.CompletedTask;
        }

        // POST: tear down whatever graph is currently live and reconstruct the named saved one
        // in its place - irreversible unless the current graph was itself saved first.
        [Route(HttpVerbs.Post, "/graphProfile/activate")]
        public Task Activate([QueryField] string name)
        {
            GraphProfile.Instance.Activate(name);
            return Task.CompletedTask;
        }
    }
}
