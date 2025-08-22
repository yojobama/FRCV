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
    internal class SinkController
    {
        // PATCH: Bind a sink to a source;
        [Route(HttpVerbs.Patch, "/bind")]
        public Task Bind([QueryField] int SinkID, [QueryField] int SourceID)
        {
            // TODO: Implement;
            return Task.CompletedTask;
        }

        // does not have to recieve a sourceId, only for multi source sinks (which do not exist in this version)
        // PATCH: Unbind a sink from a source;
        [Route(HttpVerbs.Patch, "/unbind")]
        public Task Unbind([QueryField] int SinkID, [QueryField] int? SourceID = null)
        {
            // TODO: Implement;
            return Task.CompletedTask;
        }

        // DELETE: delete a certain sink;
        [Route(HttpVerbs.Delete, "/delete")]
        public Task Delete([QueryField] int SinkID)
        {
            // TODO: Implement;
            return Task.CompletedTask;
        }
    }
}
