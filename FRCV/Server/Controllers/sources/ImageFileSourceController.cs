using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers.sources
{
    internal class ImageFileSourceController : WebApiController 
    {
        // GET: All ImageFile sources;
        [Route(EmbedIO.HttpVerbs.Get, "/get")]
        public Task<Source[]> GetAll()
        {
            List<Source> sources = new List<Source>();

            foreach (var item in SourceManager.Instance.GetAllSourceIds())
            {
                Source source = SourceManager.Instance.GetSourceById(item);
                if (source.Type == SourceType.ImageFile)
                    sources.Add(source);
            }
            return Task.FromResult(sources.ToArray());
        }

        // POST: Create ImageFile Sources from provided files;
        [Route(EmbedIO.HttpVerbs.Post, "/create")]
        public Task Create()
        {
            // TODO: Implement;
            return Task.CompletedTask;
        }
        
        
        // ---------------------------------------
        // add all sorts of things like exposure and stuff that may matter to some people
        // (look at photonvision for examples, they more or less mastered this craft)
        // ---------------------------------------

    }
}
