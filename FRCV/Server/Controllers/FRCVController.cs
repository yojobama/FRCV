using EmbedIO.Routing;
using Swan.Logging;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers
{
    internal class FRCVController
    {
        Manager _manager;

        public FRCVController(Manager manager)
        {
            _manager = manager;
            // Constructor logic here
        }

        // API endpoint definitions
        // ------------------------------------------------

        // Get: /api/frcv/getAllSinkIds
        // Get: /api/frcv/getSinkById/{id}

        // Get: /api/frcv/getAllSourceIds
        // Get: /api/frcv/getSourceById/{id}

        // Post: /api/frcv/addSink
        // Post: /api/frcv/addSource

        // Put: /api/frcv/updateSink
        // Put: /api/frcv/updateSource

        // Delete: /api/frcv/deleteSink/{id}
        // Delete: /api/frcv/deleteSource/{id}

        // Put: /api/frcv/enableSink/{id}
        // Put: /api/frcv/enableSource/{id}

        // Put: /api/frcv/disableSink/{id}
        // Put: /api/frcv/disableSource/{id}

        // Put: /api/frcv/enableAllSinks
        // Put: /api/frcv/enableAllSources

        // Put: /api/frcv/bindSinkToSource/{sinkId}/{sourceId}
        // Put: /api/frcv/unbindSinkFromSource/{sinkId}/{sourceId}

        // Get: /api/frcv/getAllLogs

        // Get: /api/frcv/getAllCurrentResults

        // Put: /api/frcv/startUDPStream
        // Put: /api/frcv/stopUDPStream
        // ------------------------------------------------
    }
}
