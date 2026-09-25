using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sinks
{
    internal class ObjectDetectionSinkController : ControllerBase
    {
        // POST: create an object detection sink bound to a previously uploaded model. The
        // backend (ONNX Runtime vs RKNN/NPU) follows from the model itself (see
        // Model.Provider's own comment) - there is no separate provider parameter to pass here.
        [HttpPost("objectDetectionSink/create")]
        public Task<int> Create([FromQuery] string name, [FromQuery] int modelId)
        {
            int sinkId = SinkManager.Instance.AddObjectDetectionSink(name, modelId);
            return Task.FromResult(sinkId);
        }

        // GET: which backend an existing sink is actually running - informational only, see
        // ObjectDetectionSink::GetBackendName's own comment for why there's no PATCH to switch it.
        [HttpGet("objectDetectionSink/backend")]
        public Task<string> GetBackend([FromQuery] int sinkId)
        {
            return Task.FromResult(SinkManager.Instance.GetObjectDetectionSinkBackendName(sinkId));
        }
    }
}
