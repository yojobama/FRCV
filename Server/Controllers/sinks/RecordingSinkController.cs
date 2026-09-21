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
    internal class RecordingSinkController : WebApiController
    {
        // POST: Create a recording sink;
        [Route(HttpVerbs.Post, "/sink/recording/create")]
        public Task<int> Create([QueryField] string Name, [QueryField] string FolderPath, [QueryField] int MaxFileCount, [QueryField] long MaxFolderSizeBytes)
        {
            throw new NotImplementedException();
            // var newSink = SinkManager.Instance.CreateRecordingSink(Name, FolderPath, MaxFileCount, MaxFolderSizeBytes);
            DB.Instance.Save(); // Save changes to the database
            return Task.FromResult(/*newSink.ID*/ 0);
        }
        // DELETE: remove a certain video;


        // ---------------------------------------
        // add a way to get the video;
        // also wanted is a way to transfer the video to be a source without manually downloading it;
        // plus a way to record a preview from a certain sink;
        // ---------------------------------------
    }
}
