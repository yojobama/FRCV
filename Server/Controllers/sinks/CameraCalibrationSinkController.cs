using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    internal class CameraCalibrationSinkController : WebApiController
    {
        // POST: Create a camera calibration sink (default 6x9 checkerboard, 25mm squares)
        [Route(HttpVerbs.Post, "/cameraCalibrationSink/create")]
        public Task<int> Create([QueryField] string name)
        {
            int sinkId = SinkManager.Instance.AddSink(name, "cameracalibrationsink");
            DB.Instance.Save();
            return Task.FromResult(sinkId);
        }

        // POST: Create a camera calibration sink with an explicit board configuration
        [Route(HttpVerbs.Post, "/cameraCalibrationSink/createWithBoard")]
        public Task<int> CreateWithBoard([QueryField] string name, [QueryField] CalibrationBoardType boardType,
            [QueryField] int rows, [QueryField] int cols, [QueryField] double squareSizeMeters,
            [QueryField] double markerSizeMeters = 0.018, [QueryField] int arucoDictionaryId = 10)
        {
            int sinkId = SinkManager.Instance.AddCameraCalibrationSinkWithBoard(
                name, boardType, rows, cols, (float)squareSizeMeters, (float)markerSizeMeters, arucoDictionaryId);
            return Task.FromResult(sinkId);
        }

        // POST: Save the checkerboard/ChArUco corners detected in the sink's latest frame, to be
        // used in the calibration phase once enough snapshots have been collected
        [Route(HttpVerbs.Post, "/cameraCalibrationSink/{id}/saveDetection")]
        public Task<bool> SaveDetection(int id)
        {
            return Task.FromResult(SinkManager.Instance.SaveCameraCalibrationBoardDetection(id));
        }

        // GET: how many snapshots have been saved so far
        [Route(HttpVerbs.Get, "/cameraCalibrationSink/{id}/snapshotCount")]
        public Task<int> GetSnapshotCount(int id)
        {
            return Task.FromResult(SinkManager.Instance.GetCameraCalibrationSnapshotCount(id));
        }

        // DELETE: remove one saved snapshot by index
        [Route(HttpVerbs.Delete, "/cameraCalibrationSink/{id}/snapshot")]
        public Task<bool> RemoveSnapshot(int id, [QueryField] int index)
        {
            return Task.FromResult(SinkManager.Instance.RemoveCameraCalibrationSnapshot(id, index));
        }

        // DELETE: discard every saved snapshot
        [Route(HttpVerbs.Delete, "/cameraCalibrationSink/{id}/snapshots")]
        public Task ClearSnapshots(int id)
        {
            SinkManager.Instance.ClearCameraCalibrationSnapshots(id);
            return Task.CompletedTask;
        }

        // POST: explicitly run cv::calibrateCamera over every saved snapshot; the UI decides
        // when this happens rather than it running implicitly on every result fetch. Also
        // persists the result (keyed by the bound camera's device path + resolution) if the
        // sink is bound to a camera source.
        [Route(HttpVerbs.Post, "/cameraCalibrationSink/{id}/run")]
        public Task<CameraCalibrationResult> RunCalibration(int id)
        {
            return Task.FromResult(SinkManager.Instance.RunCameraCalibration(id));
        }

        // GET: Retrieve the last calibration result computed by a camera calibration sink
        // (does NOT run calibration - call POST .../run first)
        [Route(HttpVerbs.Get, "/cameraCalibrationSink/{id}/result")]
        public Task<CameraCalibrationResult> GetResult(int id)
        {
            return Task.FromResult(SinkManager.Instance.GetCameraCalibrationResult(id));
        }

        // GET: every calibration result ever saved to disk, across all cameras
        [Route(HttpVerbs.Get, "/cameraCalibrationSink/savedResults")]
        public Task<List<StoredCalibration>> GetSavedResults()
        {
            return Task.FromResult(CalibrationManager.Instance.GetAll());
        }
    }
}
