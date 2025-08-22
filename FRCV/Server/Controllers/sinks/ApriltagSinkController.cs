using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    internal class ApriltagSinkController : WebApiController
    {
        // POST: Create an Apriltag sink
        [Route(HttpVerbs.Post, "/create")]
        public Task Create()
        {
            // TODO: Implement;
            return Task.CompletedTask;
        }

        // --?-- GET: Acceletation type (cpu, vulkan);

        // --?-- PATCH: Apriltag Family Type;
    }
}
