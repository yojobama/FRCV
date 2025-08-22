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
        public Task<Source> GetAll()
        {
            // TODO: Implement;
            return null;
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
