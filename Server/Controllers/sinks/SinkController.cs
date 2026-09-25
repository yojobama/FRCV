using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sinks
{
    internal class SinkController : ControllerBase
    {
        // GET: Get a sink's active status by id;
        [HttpGet("sink/getStatus")]
        public Task<bool> GetStatus([FromQuery] int SinkID)
        {
            return Task.FromResult(SinkManager.Instance.IsSinkRunning(SinkID));
        }

        // Encoding.UTF8 writes a BOM preamble, which breaks strict JSON parsers (confirmed the
        // hard way fixing OpenApiController's own /openapi.json endpoint) - reused here for the
        // same reason.
        private static readonly Encoding Utf8NoBom = new UTF8Encoding(false);

        // Written as a raw string body rather than returned as Task<string> - EmbedIO's default
        // serializer (Swan.Formatters.Json, not System.Text.Json - see OpenApiController's own
        // comment on this) re-wraps a returned string in an OUTER JSON string layer, but does so
        // WITHOUT escaping the embedded quotes the inner JSON already has, producing literally
        // invalid JSON on the wire for any result containing a nested object or string (which is
        // effectively every real result - confirmed the hard way: this endpoint went completely
        // unexercised by the webui until ROADMAP.md Phase 8c actually started calling it, so the
        // bug had been latent since this route was first written). GetResult/GetAllResults
        // already return a fully-formed JSON document as a string (Manager::GetSinkResult /
        // GetAllSinkResults both call nlohmann::json::dump()) - there is no "string value" to
        // encode here, the string already IS the response body.
        //
        // GET: the latest result JSON produced by a sink that is also a source (ApriltagSink,
        // CameraCalibrationSink, ObjectDetectionSink) - "{}" for a terminal sink (NetworkTables,
        // WebRTC, Recording) or one that hasn't produced anything yet
        [HttpGet("sink/getResult")]
        public async Task GetResult([FromQuery] int SinkID)
        {
            string json = SinkManager.Instance.GetResult(SinkID);
            await HttpContext.SendStringAsync(json, "application/json", Utf8NoBom);
        }

        // GET: every sink's latest result, keyed by sink id, as one JSON object
        [HttpGet("sink/getAllResults")]
        public async Task GetAllResults()
        {
            string json = SinkManager.Instance.GetAllResults();
            await HttpContext.SendStringAsync(json, "application/json", Utf8NoBom);
        }

        // PATCH: Enable/Disable a sink;
        [HttpPatch("sink/toggle")]
        public Task Toggle([FromQuery] int SinkID, [FromQuery] bool Enabled)
        {
            if (!Enabled) SinkManager.Instance.DisableSinkById(SinkID);
            else SinkManager.Instance.EnableSinkById(SinkID);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // PATCH: Change the name of a sink;
        [HttpPatch("sink/rename")]
        public Task Rename([FromQuery] int SinkID, [FromQuery] string NewName)
        {
            SinkManager.Instance.SetSinkName(SinkID, NewName);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // PATCH: Bind a sink to a source;
        [HttpPatch("sink/bind")]
        public Task Bind([FromQuery] int SinkID, [FromQuery] int SourceID)
        {
            SinkManager.Instance.BindSourceToSink(SinkID, SourceID);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // does not have to recieve a sourceId, only for multi source sinks (which do not exist in this version)
        // PATCH: Unbind a sink from a source;
        [HttpPatch("sink/unbind")]
        public Task Unbind([FromQuery] int SinkID, [FromQuery] int? SourceID = null)
        {
            SinkManager.Instance.UnbindSourceFromSink(SinkID, SourceID);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // DELETE: delete a certain sink;
        [HttpDelete("sink/delete")]
        public Task Delete([FromQuery] int SinkID)
        {
            try
            {
                SinkManager.Instance.DeleteSink(SinkID);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
                return Task.FromException(ex);
            }
            return Task.CompletedTask;
        }

        // GET: get all sinks;
        [HttpGet("sink/getAll")]
        public Task<List<Sink>> GetAll()
        {
            List<Sink> sinks = new List<Sink>();
            foreach (var sink in SinkManager.Instance.getAllSinkIds())
            {
                sinks.Add(SinkManager.Instance.GetSinkById(sink));
            }
            return Task.FromResult(sinks);
        }

        // PATCH: toggle driver mode on a detection sink (ApriltagDetector/ObjectDetectionSink) -
        // ROADMAP.md Phase 7. Still streams video, just skips the actual detection/NT4 publish
        // work - throws (404-equivalent via EmbedIO's own exception handling) if SinkID doesn't
        // support it, matching /sink/bind's own error-propagation style below.
        [HttpPatch("sink/driverMode")]
        public Task SetDriverMode([FromQuery] int SinkID, [FromQuery] bool Enabled)
        {
            ManagerWrapper.Instance.SetDriverMode(SinkID, Enabled);
            return Task.CompletedTask;
        }

        // GET: whether a detection sink currently has driver mode enabled
        [HttpGet("sink/driverMode")]
        public Task<bool> GetDriverMode([FromQuery] int SinkID)
        {
            return Task.FromResult(ManagerWrapper.Instance.GetDriverMode(SinkID));
        }
    }
}
