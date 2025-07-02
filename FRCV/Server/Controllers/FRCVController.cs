using EmbedIO;
using EmbedIO.Routing;
using System.Diagnostics.Contracts;

namespace Server.Controllers
{
    internal class FRCVController
    {
        Manager _manager;
        DB _db;

        public FRCVController(Manager manager, DB db)
        {
            _manager = manager;
            _db = db;
            // Constructor logic here
        }

        // API endpoint definitions
        // ------------------------------------------------

        // Get: /api/frcv/getAllSinkIds
        [Route(HttpVerbs.Get, "/api/frcv/getAllSinkIds")]
        public int[] GetAllSinkIds()
        {
            return null;
        }
        // Get: /api/frcv/getSinkById/{id}
        [Route(HttpVerbs.Get, "/api/frcv/getSinkById/{id}")]
        public Sink GetSinkById(int id)
        {
            return null;
        }

        // Get: /api/frcv/getAllSourceIds
        [Route(HttpVerbs.Get, "/api/frcv/getAllSourceIds")]
        public int[] GetAllSourceIds()
        {
            return null;
        }
        // Get: /api/frcv/getSourceById/{id}
        [Route(HttpVerbs.Get, "/api/frcv/getSourceById/{id}")]
        public Source GetSourceById(int id)
        {
            return null;
        }

        // Post: /api/frcv/addSink
        [Route(HttpVerbs.Post, "/api/frcv/addSink")]
        public void AddSink(Sink sink)
        {
            // Logic to add a sink
        }
        // Post: /api/frcv/addSource
        [Route(HttpVerbs.Post, "/api/frcv/addSource")]
        public void AddSource(Source source)
        {
            // Logic to add a source
        }

        // Put: /api/frcv/updateSink
        [Route(HttpVerbs.Put, "/api/frcv/updateSink")]
        public void UpdateSink(Sink sink)
        {
            // Logic to update a sink
        }
        // Put: /api/frcv/updateSource
        [Route(HttpVerbs.Put, "/api/frcv/updateSource")]
        public void UpdateSource(Source source)
        {
            // Logic to update a source
        }

        // Delete: /api/frcv/deleteSink/{id}
        [Route(HttpVerbs.Delete, "/api/frcv/deleteSink/{id}")]
        public void DeleteSink(int id)
        {
            // Logic to delete a sink
        }
        // Delete: /api/frcv/deleteSource/{id}
        [Route(HttpVerbs.Delete, "/api/frcv/deleteSource/{id}")]
        public void DeleteSource(int id)
        {
            // Logic to delete a source
        }

        // Put: /api/frcv/enableSink/{id}
        [Route(HttpVerbs.Put, "/api/frcv/enableSink/{id}")]
        public void EnableSink(int id)
        {
            // Logic to enable a sink
        }
        // Put: /api/frcv/enableSource/{id}
        [Route(HttpVerbs.Put, "/api/frcv/enableSource/{id}")]
        public void EnableSource(int id)
        {
            // Logic to enable a source
        }

        // Put: /api/frcv/disableSink/{id}
        [Route(HttpVerbs.Put, "/api/frcv/disableSink/{id}")]
        public void DisableSink(int id)
        {
            // Logic to disable a sink
        }
        // Put: /api/frcv/disableSource/{id}
        [Route(HttpVerbs.Put, "/api/frcv/disableSource/{id}")]
        public void DisableSource(int id)
        {
            // Logic to disable a source
        }

        // Put: /api/frcv/enableAllSinks
        [Route(HttpVerbs.Put, "/api/frcv/enableAllSinks")]
        public void EnableAllSinks()
        {
            // Logic to enable all sinks
        }
        // Put: /api/frcv/enableAllSources
        [Route(HttpVerbs.Put, "/api/frcv/enableAllSources")]
        public void EnableAllSources()
        {
            // Logic to enable all sources
        }

        // Put: /api/frcv/bindSinkToSource/{sinkId}/{sourceId}
        [Route(HttpVerbs.Put, "/api/frcv/bindSinkToSource/{sinkId}/{sourceId}")]
        public void BindSinkToSource(int sinkId, int sourceId)
        {
            // Logic to bind a sink to a source
        }
        // Put: /api/frcv/unbindSinkFromSource/{sinkId}/{sourceId}
        [Route(HttpVerbs.Put, "/api/frcv/unbindSinkFromSource/{sinkId}/{sourceId}")]
        public void UnbindSinkFromSource(int sinkId, int sourceId)
        {
            // Logic to unbind a sink from a source
        }

        // Get: /api/frcv/getAllLogs
        [Route(HttpVerbs.Get, "/api/frcv/getAllLogs")]
        public string[] GetAllLogs()
        {
            // Logic to retrieve all logs
            return null;
        }

        // Get: /api/frcv/getAllCurrentResults
        [Route(HttpVerbs.Get, "/api/frcv/getAllCurrentResults")]
        public string[] GetAllCurrentResults()
        {
            // Logic to retrieve all current results
            return null;
        }

        // Put: /api/frcv/startUDPStream
        [Route(HttpVerbs.Put, "/api/frcv/startUDPStream")]
        public void StartUDPStream()
        {
            // Logic to start a UDP stream
        }
        // Put: /api/frcv/stopUDPStream
        [Route(HttpVerbs.Put, "/api/frcv/stopUDPStream")]
        public void StopUDPStream()
        {
            // Logic to stop a UDP stream
        }
        // ------------------------------------------------
    }
}
