using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers.sinks
{
    internal class CameraCalibrationSinkController : WebApiController
    {
        // POST: Create a camera calibration sink
        [Route(HttpVerbs.Post, "/cameraCalibrationSink/create")]
        public Task<int> Create([QueryField] string name)
        {
            int sinkId = SinkManager.Instance.AddSink(name, "cameracalibrationsink");
            DB.Instance.Save();
            return Task.FromResult(sinkId);
        }

        // POST: Save the checkerboard corners detected in the sink's latest frame, to be used in the
        // calibration phase once enough snapshots have been collected
        [Route(HttpVerbs.Post, "/cameraCalibrationSink/{id}/saveDetection")]
        public Task<bool> SaveDetection(int id)
        {
            return Task.FromResult(SinkManager.Instance.SaveCameraCalibrationBoardDetection(id));
        }

        // GET: Retrieve the calibration result computed by a camera calibration sink
        [Route(HttpVerbs.Get, "/cameraCalibrationSink/{id}/result")]
        public Task<CameraCalibrationResult> GetResult(int id)
        {
            return Task.FromResult(SinkManager.Instance.GetCameraCalibrationResult(id));
        }
    }
}
