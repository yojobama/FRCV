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
        // GET: Source activation status
        [Route(EmbedIO.HttpVerbs.Get, "/source/isActive")]
        public Task<bool> IsActive([QueryField] int SourceID)
        {
            bool status = SourceManager.Instance.IsSourceActive(SourceID);
            return Task.FromResult(status);
        }

        // GET: All registered sources;
        [Route(EmbedIO.HttpVerbs.Get, "/source/getAll")]
        public Task<Source[]> GetAllSources()
        {
            List<Source> sources = new List<Source>();
            
            foreach (var item in SourceManager.Instance.GetAllSourceIds())
                sources.Add(SourceManager.Instance.GetSourceById(item));
            
            return Task.FromResult(sources.ToArray());
        }

        // PATCH: Rename am ImageFile source;
        [Route(EmbedIO.HttpVerbs.Patch, "/source/rename")]
        public Task Rename([QueryField] int SourceID, [QueryField] string newName)
        {
            SourceManager.Instance.ChangeSourceName(SourceID, newName);
            return Task.CompletedTask;
        }

        // DELETE: delete a source;
        [Route(EmbedIO.HttpVerbs.Delete, "/source/delete")]
        public Task Delete([QueryField] int SourceID)
        {
            SourceManager.Instance.DeleteSource(SourceID);
            return Task.CompletedTask;
        }
    }
}
