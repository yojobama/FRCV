using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    // See STEREO_IMPLEMENTATION_PLAN.md ss10.3/ss10.5.
    internal class StereoDepthSinkController : WebApiController
    {
        // POST: create a StereoDepthNode. `calibration` is normally the result of a
        // StereoCalibrationSink's /run (or /result), passed straight through - see
        // StereoCalibrationSinkController. Bind its left/right sources afterwards via /bind.
        [Route(HttpVerbs.Post, "/stereoDepthSink/create")]
        public Task<int> Create([QueryField] string name, [QueryField] StereoDepthBackendKind backend,
            [QueryField] double minDepthMeters, [QueryField] double maxDepthMeters,
            [QueryField] int maxSkewUs, [QueryField] StereoFrameOutput frameOutput,
            [JsonData] StereoCalibrationResultDto calibration)
        {
            int sinkId = SinkManager.Instance.AddStereoDepthSink(name, backend, calibration.ToNative(), minDepthMeters, maxDepthMeters, maxSkewUs, frameOutput);
            return Task.FromResult(sinkId);
        }

        // PATCH: bind the explicit left/right camera sources for this stereo sink
        [Route(HttpVerbs.Patch, "/stereoDepthSink/{id}/bind")]
        public Task Bind(int id, [QueryField] int leftSourceId, [QueryField] int rightSourceId)
        {
            SinkManager.Instance.BindStereoSourcesToSink(id, leftSourceId, rightSourceId);
            return Task.CompletedTask;
        }

        // GET: which backend actually ended up running (e.g. "lavc_sw", "rkmpp_hwenc", "sgbm")
        [Route(HttpVerbs.Get, "/stereoDepthSink/{id}/backendName")]
        public Task<string> GetBackendName(int id)
        {
            return Task.FromResult(SinkManager.Instance.GetStereoDepthBackendName(id));
        }

        // GET: summary stats from the most recently processed pair - the full per-block grid is
        // in /sink/getResult's JSON (capped in size, see StereoDepthNode's own "Outputs" note),
        // not here.
        [Route(HttpVerbs.Get, "/stereoDepthSink/{id}/stats")]
        public Task<StereoDepthStatsDto> GetStats(int id)
        {
            return Task.FromResult(new StereoDepthStatsDto(
                SinkManager.Instance.GetStereoDepthValidFraction(id),
                SinkManager.Instance.GetStereoDepthMedianDepthMeters(id)));
        }
    }
}
