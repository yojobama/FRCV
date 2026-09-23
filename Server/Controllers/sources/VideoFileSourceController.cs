using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using HttpMultipartParser;
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
        [Route(HttpVerbs.Get, "/videoFileSource/getAll")]
        public Task<Source[]> GetAll()
        {
            List<Source> sources = new List<Source>();

            foreach (var item in SourceManager.Instance.GetAllSourceIds())
            {
                Source source = SourceManager.Instance.GetSourceById(item);
                if (source.Type == SourceType.VideoFile)
                    sources.Add(source);
            }
            return Task.FromResult(sources.ToArray());
        }

        // POST: Create VideoFile Sources from all provided files with a default FPS of 30;
        [Route(HttpVerbs.Post, "/videoFileSource/create")]
        public Task<int[]> Create()
        {
            // Logic to handle video file upload
            var parser = MultipartFormDataParser.Parse(HttpContext.OpenRequestStream());
            List<int> created = new List<int>();

            foreach(var file in parser.Files)
            {
                if (file != null)
                {
                    string fileName = file.FileName;
                    Stream fileStream = file.Data;

                    Directory.CreateDirectory("videos");

                    string savedPath = Path.Combine("videos", fileName);
                    using (var output = File.Create(savedPath))
                    {
                        fileStream.CopyTo(output);
                    }
                    // same "native code must not open this file until the upload's own
                    // FileStream has actually closed" fix as ImageFileSourceController.Create -
                    // see its own comment.
                    created.Add(SourceManager.Instance.InitializeVideoFileSource(savedPath, 30, Path.GetFileNameWithoutExtension(fileName)));
                }
            }
            return Task.FromResult(created.ToArray());
        }

        // PATCH: Change VideoFile FPS;
        [Route(HttpVerbs.Patch, "/videoFileSource/changeFPS")]
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
