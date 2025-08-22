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
    internal class CameraSourceController : WebApiController
    {
        // POST: create camera sources from all connected cameras;
        [Route(EmbedIO.HttpVerbs.Post, "/createAll")]
        public Task CreateAll()
        {
            // TODO: Implement;
            return Task.CompletedTask;
        }

        // POST: Create a camera source from a specified camera;
        [Route(EmbedIO.HttpVerbs.Post, "/create")]
        public Task Create([QueryData] CameraHardwareInfo hardwareInfo)
        {
            // TODO: Implement;
            return Task.CompletedTask;
        }

        // GET: All connected cameras;
        [Route(HttpVerbs.Get, "/getRegistered")]
        public Task<Sink> GetAllConnected()
        {
            // TODO: Implement;
            return null;
        }

        // GET: All available camers that have not yet been turns into a source;
        [Route(HttpVerbs.Get, "/getNotRegistered")]
        public Task<CameraHardwareInfo> GetNotRegistered()
        {
            // TODO: Implement;
            return null;
        }

        // ---------------------------------------
        // add all sorts of things like exposure and stuff that may matter to some people
        // (look at photonvision for examples, they more or less mastered this craft)
        // ---------------------------------------
    }
}
