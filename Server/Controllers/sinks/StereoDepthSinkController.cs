using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sinks
{
    // See STEREO_IMPLEMENTATION_PLAN.md ss10.3/ss10.5.
    internal class StereoDepthSinkController : ControllerBase
    {
        // POST: create a StereoDepthNode. `calibration` is normally the result of a
        // StereoCalibrationSink's /run (or /result), passed straight through - see
        // StereoCalibrationSinkController. Bind its left/right sources afterwards via /bind.
        // Body read and deserialized by hand with System.Text.Json rather than [JsonData] - see
        // CameraSourceController.Create's comment on why (Swan can't construct a record struct's
        // primary constructor and silently leaves every field at its default).
        [HttpPost("stereoDepthSink/create")]
        public async Task<int> Create([FromQuery] string name, [FromQuery] StereoDepthBackendKind backend,
            [FromQuery] double minDepthMeters, [FromQuery] double maxDepthMeters,
            [FromQuery] int maxSkewUs, [FromQuery] StereoFrameOutput frameOutput)
        {
            string body = await HttpContext.GetRequestBodyAsStringAsync();
            StereoCalibrationResultDto calibration = System.Text.Json.JsonSerializer.Deserialize<StereoCalibrationResultDto>(body);
            int sinkId = SinkManager.Instance.AddStereoDepthSink(name, backend, calibration.ToNative(), minDepthMeters, maxDepthMeters, maxSkewUs, frameOutput);
            return sinkId;
        }

        // PATCH: bind the explicit left/right camera sources for this stereo sink
        [HttpPatch("stereoDepthSink/{id}/bind")]
        public Task Bind(int id, [FromQuery] int leftSourceId, [FromQuery] int rightSourceId)
        {
            SinkManager.Instance.BindStereoSourcesToSink(id, leftSourceId, rightSourceId);
            return Task.CompletedTask;
        }

        // GET: which backend actually ended up running (e.g. "lavc_sw", "rkmpp_hwenc", "sgbm")
        [HttpGet("stereoDepthSink/{id}/backendName")]
        public Task<string> GetBackendName(int id)
        {
            return Task.FromResult(SinkManager.Instance.GetStereoDepthBackendName(id));
        }

        // GET: summary stats from the most recently processed pair - the full per-block grid is
        // in /sink/getResult's JSON (capped in size, see StereoDepthNode's own "Outputs" note),
        // not here.
        [HttpGet("stereoDepthSink/{id}/stats")]
        public Task<StereoDepthStatsDto> GetStats(int id)
        {
            return Task.FromResult(new StereoDepthStatsDto(
                SinkManager.Instance.GetStereoDepthValidFraction(id),
                SinkManager.Instance.GetStereoDepthMedianDepthMeters(id)));
        }
    }
}
