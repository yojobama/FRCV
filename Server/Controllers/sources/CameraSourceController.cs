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
        [Route(EmbedIO.HttpVerbs.Post, "/cameraSource/createAll")]
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
        // Body read and deserialized by hand with System.Text.Json rather than [JsonData] - the
        // latter goes through Swan.Formatters, which cannot construct a record struct's primary
        // constructor and silently leaves every field at its default (confirmed live: every
        // [JsonData]-bound record struct across this controller and StereoDepthSinkController
        // threw ArgumentNullException from inside the native setter, regardless of the JSON
        // body's field casing - not a casing bug, Swan just never calls the constructor at all).
        [Route(EmbedIO.HttpVerbs.Post, "/cameraSource/create")]
        public async Task<int> Create([QueryField] string name = "default")
        {
            string body = await HttpContext.GetRequestBodyAsStringAsync();
            CameraHardwareInfoDto hardwareInfo = System.Text.Json.JsonSerializer.Deserialize<CameraHardwareInfoDto>(body);
            int sourceId = SourceManager.Instance.InitializeCameraSource(hardwareInfo.ToNative(), name);
            return sourceId;
        }

        // GET: All connected cameras;
        [Route(HttpVerbs.Get, "/cameraSource/getRegistered")]
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
        [Route(HttpVerbs.Get, "/cameraSource/getNotRegistered")]
        public Task<CameraHardwareInfoDto[]> GetNotRegistered()
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

            return Task.FromResult(cameraHardwareInfoArray.Select(CameraHardwareInfoDto.From).ToArray());
        }

        // GET: every capture mode this camera actually advertises (ROADMAP.md Phase 3b).
        [Route(HttpVerbs.Get, "/cameraSource/{id}/modes")]
        public Task<CameraModeDto[]> GetModes(int id)
        {
            return Task.FromResult(ManagerWrapper.Instance.GetCameraModes(id).Select(CameraModeDto.From).ToArray());
        }

        // GET: what the device is actually running right now - check isNative after a /mode PATCH
        // to see whether the request was honoured exactly or silently substituted (both V4L2 and
        // Media Foundation do this - see CameraMode's own comment).
        [Route(HttpVerbs.Get, "/cameraSource/{id}/currentMode")]
        public Task<CameraModeDto> GetCurrentMode(int id)
        {
            return Task.FromResult(CameraModeDto.From(ManagerWrapper.Instance.GetCameraCurrentMode(id)));
        }

        // PATCH: request an explicit capture mode. Returns whether the underlying ioctl/API call
        // itself succeeded - NOT whether the device honoured it exactly; re-GET /currentMode
        // afterwards for that.
        [Route(HttpVerbs.Patch, "/cameraSource/{id}/mode")]
        public async Task<bool> SetMode(int id)
        {
            string body = await HttpContext.GetRequestBodyAsStringAsync();
            CameraModeDto mode = System.Text.Json.JsonSerializer.Deserialize<CameraModeDto>(body);
            return ManagerWrapper.Instance.SetCameraMode(id, mode.ToNative());
        }

        // PATCH: exposure/gain control - a fixed short exposure is what actually makes AprilTags
        // detect reliably on a moving robot. Call autoExposure=false before exposureAbsolute for
        // the exposure value to actually take effect on most UVC hardware.
        [Route(HttpVerbs.Patch, "/cameraSource/{id}/exposure")]
        public Task<bool> SetExposure(int id, [QueryField] int exposureAbsolute)
        {
            return Task.FromResult(ManagerWrapper.Instance.SetCameraExposure(id, exposureAbsolute));
        }

        [Route(HttpVerbs.Patch, "/cameraSource/{id}/autoExposure")]
        public Task<bool> SetAutoExposure(int id, [QueryField] bool enabled)
        {
            return Task.FromResult(ManagerWrapper.Instance.SetCameraAutoExposure(id, enabled));
        }

        [Route(HttpVerbs.Patch, "/cameraSource/{id}/gain")]
        public Task<bool> SetGain(int id, [QueryField] int gain)
        {
            return Task.FromResult(ManagerWrapper.Instance.SetCameraGain(id, gain));
        }

        // POST: split this camera's frame into a fixed crop, published as its own independent
        // source (ROADMAP.md Phase 3d) - the side-by-side/top-bottom stereo building block. Call
        // this twice against one side-by-side camera (left half, right half) to get two ordinary
        // sources bindable into a stereo sink exactly like two real cameras.
        [Route(HttpVerbs.Post, "/cameraSource/{id}/roi")]
        public Task<int> CreateRoi(int id, [QueryField] int x, [QueryField] int y, [QueryField] int width, [QueryField] int height)
        {
            return Task.FromResult(ManagerWrapper.Instance.CreateRoiSource(id, x, y, width, height));
        }
    }
}
