using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers.sources
{
    internal class SourceController : WebApiController
    {
        // GET: All registered sources;
        [Route(EmbedIO.HttpVerbs.Get, "/getAll")]
        public Task<Source[]> GetAllSources()
        {
            // TODO: Implement;
            return null;
        }

        // PATCH: Rename am ImageFile source;
        [Route(EmbedIO.HttpVerbs.Patch, "/rename")]
        public Task Rename([QueryField] int SinkID, [QueryField] string newName)
        {
            // TODO: Implement;
            return Task.CompletedTask;
        }

        // DELETE: delete a source;
        [Route(EmbedIO.HttpVerbs.Delete, "/delete")]
        public Task Delete([QueryField] int SourceID)
        {
            return Task.CompletedTask;
        }
    }
}
