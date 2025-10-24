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
    internal class ObjectDetectionSinkController : WebApiController
    {
        // POST: create an object detection sink
        [Route(HttpVerbs.Post, "api/sinks/object-detection/create")]
        public Task CreateObjectDetectionSink()
        {
            throw new NotImplementedException();
        }
        // POST: upload a model to the sink
    }
}
