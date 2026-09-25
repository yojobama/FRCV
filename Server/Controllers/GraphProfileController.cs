using System.Collections.Generic;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers
{
    // ROADMAP.md Phase 8/E6: save/restore the whole node graph under a name - see
    // GraphProfile.cs's own comment for how this differs from the existing per-source
    // PipelineProfile.
    internal class GraphProfileController : ControllerBase
    {
        // GET: every saved whole-graph profile name, for the top-bar dropdown / LeftRail list.
        [HttpGet("graphProfile/list")]
        public Task<List<string>> List()
        {
            return Task.FromResult(GraphProfile.Instance.ListProfiles());
        }

        // POST: snapshot the CURRENTLY LIVE graph (every source, sink, and binding) under `name`
        // - overwrites any existing profile with the same name.
        [HttpPost("graphProfile/saveCurrentAs")]
        public Task SaveCurrentAs([FromQuery] string name)
        {
            GraphProfile.Instance.SaveCurrentAs(name);
            return Task.CompletedTask;
        }

        // POST: tear down whatever graph is currently live and reconstruct the named saved one
        // in its place - irreversible unless the current graph was itself saved first.
        [HttpPost("graphProfile/activate")]
        public Task Activate([FromQuery] string name)
        {
            GraphProfile.Instance.Activate(name);
            return Task.CompletedTask;
        }
    }
}
