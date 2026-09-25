using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sources
{
    internal class CameraSourceController : ControllerBase
    {
        // POST: create camera sources from all connected cameras;
        [HttpPost("cameraSource/createAll")]
        public Task CreateAll()
        {
            CameraHardwareInfo[] cameraHardwareInfoArray = GetUnregisteredCameras();

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
        [HttpPost("cameraSource/create")]
        public async Task<int> Create([FromQuery] string name = "default")
        {
            string body = await HttpContext.GetRequestBodyAsStringAsync();
            CameraHardwareInfoDto hardwareInfo = System.Text.Json.JsonSerializer.Deserialize<CameraHardwareInfoDto>(body);
            int sourceId = SourceManager.Instance.InitializeCameraSource(hardwareInfo.ToNative(), name);
            return sourceId;
        }

        // GET: All connected cameras;
        [HttpGet("cameraSource/getRegistered")]
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
        [HttpGet("cameraSource/getNotRegistered")]
        public Task<CameraHardwareInfoDto[]> GetNotRegistered()
        {
            return Task.FromResult(GetUnregisteredCameras().Select(CameraHardwareInfoDto.From).ToArray());
        }

        // Enumerated cameras no camera source already uses. Compared by the device node each path
        // resolves to, not by string: the enumerator now reports stable /dev/v4l/by-path links,
        // while a source saved before that change still holds a raw /dev/videoN for the same
        // physical camera - a string compare would offer it again as a second, "new" camera.
        private static CameraHardwareInfo[] GetUnregisteredCameras()
        {
            HashSet<string> registered = SourceManager.Instance.GetAllSourceIds()
                .Select(id => SourceManager.Instance.GetSourceById(id))
                .Where(source => source?.Type == SourceType.Camera && source.CameraHardwareInfo != null)
                .Select(source => ResolveDevicePath(source!.CameraHardwareInfo!.path))
                .ToHashSet();
            return ManagerWrapper.Instance.EnumerateAvailableCameras()
                .Where(hardwareInfo => !registered.Contains(ResolveDevicePath(hardwareInfo.path)))
                .ToArray();
        }

        private static string ResolveDevicePath(string path)
        {
            try
            {
                return System.IO.File.ResolveLinkTarget(path, returnFinalTarget: true)?.FullName ?? path;
            }
            catch (Exception)
            {
                // not a filesystem path at all (Windows camera indices), or gone - compare as-is
                return path;
            }
        }

        // GET: every capture mode this camera actually advertises (ROADMAP.md Phase 3b).
        [HttpGet("cameraSource/{id}/modes")]
        public Task<CameraModeDto[]> GetModes(int id)
        {
            return Task.FromResult(ManagerWrapper.Instance.GetCameraModes(id).Select(CameraModeDto.From).ToArray());
        }

        // GET: what the device is actually running right now - check isNative after a /mode PATCH
        // to see whether the request was honoured exactly or silently substituted (both V4L2 and
        // Media Foundation do this - see CameraMode's own comment).
        [HttpGet("cameraSource/{id}/currentMode")]
        public Task<CameraModeDto> GetCurrentMode(int id)
        {
            return Task.FromResult(CameraModeDto.From(ManagerWrapper.Instance.GetCameraCurrentMode(id)));
        }

        // GET: whether this camera has a saved calibration, and whether that calibration still
        // matches the camera's CURRENT capture mode (ROADMAP.md Phase 8/E5) - a SetMode call
        // above can silently leave a bound ApriltagDetector's pose estimation running on
        // intrinsics computed for a different resolution, with nothing else surfacing that.
        [HttpGet("cameraSource/{id}/calibrationStatus")]
        public Task<CalibrationStatusDto> GetCalibrationStatus(int id)
        {
            return Task.FromResult(CalibrationStatusDto.From(CalibrationManager.Instance.GetCalibrationStatus(id)));
        }

        // PATCH: request an explicit capture mode. Returns whether the underlying ioctl/API call
        // itself succeeded - NOT whether the device honoured it exactly; re-GET /currentMode
        // afterwards for that.
        [HttpPatch("cameraSource/{id}/mode")]
        public async Task<bool> SetMode(int id)
        {
            string body = await HttpContext.GetRequestBodyAsStringAsync();
            CameraModeDto mode = System.Text.Json.JsonSerializer.Deserialize<CameraModeDto>(body);
            return ManagerWrapper.Instance.SetCameraMode(id, mode.ToNative());
        }

        // PATCH: exposure/gain control - a fixed short exposure is what actually makes AprilTags
        // detect reliably on a moving robot. Call autoExposure=false before exposureAbsolute for
        // the exposure value to actually take effect on most UVC hardware.
        [HttpPatch("cameraSource/{id}/exposure")]
        public Task<bool> SetExposure(int id, [FromQuery] int exposureAbsolute)
        {
            return Task.FromResult(ManagerWrapper.Instance.SetCameraExposure(id, exposureAbsolute));
        }

        [HttpPatch("cameraSource/{id}/autoExposure")]
        public Task<bool> SetAutoExposure(int id, [FromQuery] bool enabled)
        {
            return Task.FromResult(ManagerWrapper.Instance.SetCameraAutoExposure(id, enabled));
        }

        [HttpPatch("cameraSource/{id}/gain")]
        public Task<bool> SetGain(int id, [FromQuery] int gain)
        {
            return Task.FromResult(ManagerWrapper.Instance.SetCameraGain(id, gain));
        }

        // GET: the device's own range and current value for the exposure/gain controls above -
        // units differ per camera (a UVC webcam's exposure is 100us steps, an Arducam MIPI
        // module's is sensor lines), so the UI bounds its inputs by this rather than assuming.
        [HttpGet("cameraSource/{id}/controls")]
        public Task<CameraControlsDto> GetControls(int id)
        {
            return Task.FromResult(new CameraControlsDto(
                CameraControlRangeDto.From(ManagerWrapper.Instance.GetCameraExposureRange(id)),
                CameraControlRangeDto.From(ManagerWrapper.Instance.GetCameraGainRange(id))));
        }

        // POST: split this camera's frame into a fixed crop, published as its own independent
        // source (ROADMAP.md Phase 3d) - the side-by-side/top-bottom stereo building block. Call
        // this twice against one side-by-side camera (left half, right half) to get two ordinary
        // sources bindable into a stereo sink exactly like two real cameras.
        [HttpPost("cameraSource/{id}/roi")]
        public Task<int> CreateRoi(int id, [FromQuery] int x, [FromQuery] int y, [FromQuery] int width, [FromQuery] int height)
        {
            return Task.FromResult(ManagerWrapper.Instance.CreateRoiSource(id, x, y, width, height));
        }
    }
}
