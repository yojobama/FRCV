using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    // See STEREO_IMPLEMENTATION_PLAN.md ss10.2/ss10.5. Mirrors CameraCalibrationSinkController's
    // shape, plus the explicit two-source bind every stereo sink needs (the generic
    // /sink/bind only takes one source and has no left/right notion at all).
    internal class StereoCalibrationSinkController : WebApiController
    {
        // POST: Create a stereo calibration sink (default 6x9 checkerboard, 25mm squares)
        [Route(HttpVerbs.Post, "/stereoCalibrationSink/create")]
        public Task<int> Create([QueryField] string name)
        {
            int sinkId = SinkManager.Instance.AddStereoCalibrationSink(name);
            DB.Instance.Save();
            return Task.FromResult(sinkId);
        }

        // POST: Create a stereo calibration sink with an explicit checkerboard configuration.
        // ChArUco is not supported for stereo (StereoCalibrator.h) - only BOARD_CHECKERBOARD.
        [Route(HttpVerbs.Post, "/stereoCalibrationSink/createWithBoard")]
        public Task<int> CreateWithBoard([QueryField] string name, [QueryField] CalibrationBoardType boardType,
            [QueryField] int rows, [QueryField] int cols, [QueryField] double squareSizeMeters)
        {
            int sinkId = SinkManager.Instance.AddStereoCalibrationSinkWithBoard(name, boardType, rows, cols, (float)squareSizeMeters);
            return Task.FromResult(sinkId);
        }

        // PATCH: bind the explicit left/right camera sources for this stereo sink
        [Route(HttpVerbs.Patch, "/stereoCalibrationSink/{id}/bind")]
        public Task Bind(int id, [QueryField] int leftSourceId, [QueryField] int rightSourceId)
        {
            SinkManager.Instance.BindStereoSourcesToSink(id, leftSourceId, rightSourceId);
            return Task.CompletedTask;
        }

        // POST: save the most recently matched (both-eyes-found, within-skew) checkerboard pair
        [Route(HttpVerbs.Post, "/stereoCalibrationSink/{id}/saveDetection")]
        public Task<bool> SaveDetection(int id)
        {
            return Task.FromResult(SinkManager.Instance.SaveStereoCalibrationDetection(id));
        }

        // GET: how many pairs have been saved so far
        [Route(HttpVerbs.Get, "/stereoCalibrationSink/{id}/pairCount")]
        public Task<int> GetPairCount(int id)
        {
            return Task.FromResult(SinkManager.Instance.GetStereoCalibrationPairCount(id));
        }

        // DELETE: remove one saved pair by index
        [Route(HttpVerbs.Delete, "/stereoCalibrationSink/{id}/pair")]
        public Task<bool> RemovePair(int id, [QueryField] int index)
        {
            return Task.FromResult(SinkManager.Instance.RemoveStereoCalibrationPair(id, index));
        }

        // DELETE: discard every saved pair
        [Route(HttpVerbs.Delete, "/stereoCalibrationSink/{id}/pairs")]
        public Task ClearPairs(int id)
        {
            SinkManager.Instance.ClearStereoCalibrationPairs(id);
            return Task.CompletedTask;
        }

        // POST: explicitly run cv::stereoCalibrate + cv::stereoRectify over every saved pair -
        // the UI decides when this happens rather than it running implicitly. Also persists the
        // result (keyed by both cameras' device paths + resolution) if bound to two real cameras.
        // Check the returned result's epipolarRms - gate real use at < 0.5px, see
        // STEREO_IMPLEMENTATION_PLAN.md ss10.2; stereoRms alone does not predict codec-stereo
        // density/validity the way epipolarRms does.
        [Route(HttpVerbs.Post, "/stereoCalibrationSink/{id}/run")]
        public Task<StereoCalibrationResultDto> RunCalibration(int id)
        {
            return Task.FromResult(StereoCalibrationResultDto.From(SinkManager.Instance.RunStereoCalibration(id)));
        }

        // GET: retrieve the last calibration result computed by this sink
        [Route(HttpVerbs.Get, "/stereoCalibrationSink/{id}/result")]
        public Task<StereoCalibrationResultDto> GetResult(int id)
        {
            return Task.FromResult(StereoCalibrationResultDto.From(SinkManager.Instance.GetStereoCalibrationResult(id)));
        }
    }
}
