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
            CameraHardwareInfo[] cameraHardwareInfoArray = ManagerWrapper.Instance.EnumerateAvailableCameras().ToArray();

            foreach (var item in SourceManager.Instance.GetAllSourceIds())
            {
                Source source = SourceManager.Instance.GetSourceById(item);
                if (source.Type == SourceType.Camera)
                {
                    cameraHardwareInfoArray = cameraHardwareInfoArray.Where(hardwareInfo => hardwareInfo.path != source.CameraHardwareInfo.path).ToArray();
                }
            }

            cameraHardwareInfoArray.ToList().ForEach(hardwareInfo => 
            {
                int sourceId = SourceManager.Instance.InitializeCameraSource(hardwareInfo);
            });

            return Task.CompletedTask;
        }

        // POST: Create a camera source from a specified camera;
        [Route(EmbedIO.HttpVerbs.Post, "/create")]
        public Task Create([JsonData] CameraHardwareInfo hardwareInfo)
        {
            int sourceId = SourceManager.Instance.InitializeCameraSource(hardwareInfo);
            return Task.FromResult(sourceId);
        }

        // GET: All connected cameras;
        [Route(HttpVerbs.Get, "/getRegistered")]
        public Task<Source[]> GetRegistered()
        {
            List<Source> sources = new List<Source>();

            foreach (var item in SourceManager.Instance.GetAllSourceIds())
            {
                Source source = SourceManager.Instance.GetSourceById(item);
                if (source.Type == SourceType.Camera)
                    sources.Add(source);
            }
            return Task.FromResult(sources.ToArray());
        }

        // GET: All available camers that have not yet been turns into a source;
        [Route(HttpVerbs.Get, "/getNotRegistered")]
        public Task<CameraHardwareInfo[]> GetNotRegistered()
        {
            CameraHardwareInfo[] cameraHardwareInfoArray = ManagerWrapper.Instance.EnumerateAvailableCameras().ToArray();
            
            foreach (var item in SourceManager.Instance.GetAllSourceIds())
            {
                Source source = SourceManager.Instance.GetSourceById(item);
                if (source.Type == SourceType.Camera)
                {
                    cameraHardwareInfoArray = cameraHardwareInfoArray.Where(hardwareInfo => hardwareInfo.path != source.CameraHardwareInfo.path).ToArray();
                }
            }

            return Task.FromResult(cameraHardwareInfoArray);
        }

        // ---------------------------------------
        // add all sorts of things like exposure and stuff that may matter to some people
        // (look at photonvision for examples, they more or less mastered this craft)
        // ---------------------------------------
    }
}
