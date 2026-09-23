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
    internal class ImageFileSourceController : WebApiController 
    {
        // GET: All ImageFile sources;
        [Route(EmbedIO.HttpVerbs.Get, "/imageFileSource/get")]
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
        [Route(EmbedIO.HttpVerbs.Post, "/imageFileSource/create")]
        public Task<int[]> Create()
        {
            // Logic to handle video file upload
            var parser = MultipartFormDataParser.Parse(HttpContext.OpenRequestStream());
            List<int> created = new List<int>();

            foreach (var file in parser.Files)
            {
                if (file != null)
                {
                    string fileName = file.FileName;
                    Stream fileStream = file.Data;

                    Directory.CreateDirectory("images");

                    string savedPath = Path.Combine("images", fileName);
                    using (var output = File.Create(savedPath))
                    {
                        fileStream.CopyTo(output);
                    }
                    // native cv::imread (initializeImageFileSource -> ImageFileSource's own
                    // constructor) must run AFTER the FileStream above is closed, not inside its
                    // `using` block - File.Create's default FileShare.None holds an exclusive
                    // lock on Windows until disposed, and a second handle (OpenCV's own fopen/
                    // CreateFile call) trying to read the SAME file while that lock is still held
                    // fails outright there ("can't open/read file: check file path/integrity").
                    // Confirmed the hard way running this natively on Windows for the first time -
                    // Linux's own file semantics have no such exclusivity, which is exactly why
                    // this went unnoticed through every WSL/Linux run this project has had so far.
                    created.Add(SourceManager.Instance.initializeImageFileSource(savedPath, Path.GetFileNameWithoutExtension(fileName)));
                }
            }
            return Task.FromResult(created.ToArray());
        }
        
        
        // ---------------------------------------
        // add all sorts of things like exposure and stuff that may matter to some people
        // (look at photonvision for examples, they more or less mastered this craft)
        // ---------------------------------------
    }
}
