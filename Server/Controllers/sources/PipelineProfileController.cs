using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sources
{
    // ROADMAP.md Phase 7: pipeline profiles - see Server/PipelineProfile.cs for the design.
    // A profile belongs to a Source (camera); activating one swaps that source's own detection
    // sink (ApriltagSink or ObjectDetectionSink) for a fresh instance built from the profile's
    // settings, leaving every other sink bound to the same source (WebRTC preview, driver mode)
    // untouched. This is what a robot program's pipelineIndex actually switches.
    internal class PipelineProfileController : ControllerBase
    {
        // POST: define a new AprilTag-detection profile on a source. calibratorSinkId is
        // optional - a profile with none gets pose estimation without real-world scale/undistort
        // until one is attached (matches CreateApriltagDetector's own "empty calibration result"
        // default). Returns the new profile's index.
        [HttpPost("source/profiles/apriltag")]
        public Task<int> CreateApriltagProfile([FromQuery] int sourceId, [FromQuery] string name,
            [FromQuery] double tagSize, [FromQuery] int? calibratorSinkId = null,
            [FromQuery] ApriltagBackendKind backend = ApriltagBackendKind.APRILTAG_BACKEND_CPU,
            [FromQuery] int frameWidth = 0, [FromQuery] int frameHeight = 0,
            [FromQuery] bool driverMode = false)
        {
            int index = SourceManager.Instance.AddApriltagProfile(sourceId, name, tagSize, calibratorSinkId,
                backend, frameWidth, frameHeight, driverMode);
            return Task.FromResult(index);
        }

        // POST: define a new object-detection profile on a source, reusing a model already
        // registered via the model-upload endpoint (see ModelManager). Returns the new profile's
        // index.
        [HttpPost("source/profiles/objectDetection")]
        public Task<int> CreateObjectDetectionProfile([FromQuery] int sourceId, [FromQuery] string name,
            [FromQuery] int modelId)
        {
            int index = SourceManager.Instance.AddObjectDetectionProfile(sourceId, name, modelId);
            return Task.FromResult(index);
        }

        // POST: upload a WPILib-format AprilTagFieldLayout JSON body onto one profile - see
        // PipelineProfile.FieldLayoutPath's own comment on why this is keyed by profile, not by
        // whatever sink id happens to be running it. Returns the number of tags loaded, or -1 if
        // the body wasn't a valid field layout.
        [HttpPost("source/profiles/fieldLayout")]
        public async Task<int> SetProfileFieldLayout([FromQuery] int sourceId, [FromQuery] int index)
        {
            using var reader = new StreamReader(HttpContext.OpenRequestStream());
            string json = await reader.ReadToEndAsync();

            string layoutDir = Path.Combine(AppContext.BaseDirectory, "fieldLayouts");
            Directory.CreateDirectory(layoutDir);
            string path = Path.Combine(layoutDir, $"source-{sourceId}-profile-{index}.json");
            await System.IO.File.WriteAllTextAsync(path, json);

            SourceManager.Instance.SetProfileFieldLayout(sourceId, index, path);

            Source source = SourceManager.Instance.GetSourceById(sourceId);
            bool isActive = source != null && source.ActiveProfileIndex == index && source.ActiveDetectionSinkId.HasValue;
            return isActive
                ? ManagerWrapper.Instance.GetFieldLayoutTagCount(source.ActiveDetectionSinkId.Value)
                : 0;
        }

        // GET: every profile defined on a source.
        [HttpGet("source/profiles")]
        public Task<List<PipelineProfile>> GetProfiles([FromQuery] int sourceId)
        {
            return Task.FromResult(SourceManager.Instance.GetProfiles(sourceId));
        }

        // GET: the currently active profile's index, or -1 if none has ever been activated -
        // this is the coprocessor-side equivalent of PhotonVision's own readable pipelineIndex.
        [HttpGet("source/profiles/active")]
        public Task<int> GetActiveProfile([FromQuery] int sourceId)
        {
            return Task.FromResult(SourceManager.Instance.GetActiveProfileIndex(sourceId));
        }

        // PATCH: switch which profile is running for a source - PhotonVision's setPipelineIndex
        // equivalent.
        [HttpPatch("source/profiles/activate")]
        public Task Activate([FromQuery] int sourceId, [FromQuery] int index)
        {
            SourceManager.Instance.ActivateProfile(sourceId, index);
            return Task.CompletedTask;
        }

        // DELETE: remove a profile definition. Refuses to delete the currently active one -
        // activate a different profile first.
        [HttpDelete("source/profiles")]
        public Task DeleteProfile([FromQuery] int sourceId, [FromQuery] int index)
        {
            SourceManager.Instance.DeleteProfile(sourceId, index);
            return Task.CompletedTask;
        }
    }
}
