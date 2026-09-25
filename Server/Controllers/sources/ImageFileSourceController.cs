using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sources
{
    internal class ImageFileSourceController : ControllerBase 
    {
        // GET: All ImageFile sources;
        [HttpGet("imageFileSource/get")]
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
        [HttpPost("imageFileSource/create")]
        public async Task<int[]> Create()
        {
            var form = await Request.ReadFormAsync(HttpContext.RequestAborted);
            List<int> created = new List<int>();

            foreach (var file in form.Files)
            {
                if (file != null)
                {
                    // GetFileName: the client-supplied name is untrusted - never let it escape images/
                    string fileName = Path.GetFileName(file.FileName);

                    Directory.CreateDirectory("images");

                    string savedPath = Path.Combine("images", fileName);
                    using (var output = System.IO.File.Create(savedPath))
                    {
                        await file.CopyToAsync(output, HttpContext.RequestAborted);
                    }
                    // native cv::imread (initializeImageFileSource -> ImageFileSource's own
                    // constructor) must run AFTER the FileStream above is closed, not inside its
                    // `using` block - System.IO.File.Create's default FileShare.None holds an exclusive
                    // lock on Windows until disposed, and a second handle (OpenCV's own fopen/
                    // CreateFile call) trying to read the SAME file while that lock is still held
                    // fails outright there ("can't open/read file: check file path/integrity").
                    // Confirmed the hard way running this natively on Windows for the first time -
                    // Linux's own file semantics have no such exclusivity, which is exactly why
                    // this went unnoticed through every WSL/Linux run this project has had so far.
                    created.Add(SourceManager.Instance.initializeImageFileSource(savedPath, Path.GetFileNameWithoutExtension(fileName)));
                }
            }
            return created.ToArray();
        }

        
        // ---------------------------------------
        // add all sorts of things like exposure and stuff that may matter to some people
        // (look at photonvision for examples, they more or less mastered this craft)
        // ---------------------------------------
    }
}
