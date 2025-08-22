using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers.sources
{
    internal class VideoFileSourceController : WebApiController
    {
        // GET: All VideoFile sources;
        [Route(HttpVerbs.Get, "/getAll")]
        public Task<Source> GetAll()
        {
            // TODO: Implement;
            return null;
        }

        // POST: Create VideoFile Sources from all provided files with a default FPS of 30;
        [Route(HttpVerbs.Post, "/create")]
        public Task Create()
        {
            // TODO: Implement;
            return Task.CompletedTask;
        }

        // PATCH: Change VideoFile FPS;
        [Route(HttpVerbs.Patch, "changeFPS")]
        public Task ChangeFPS([QueryField] int fps)
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
