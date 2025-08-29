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
    internal class SinkController : WebApiController
    {
        // PATCH: Bind a sink to a source;
        [Route(HttpVerbs.Patch, "/bind")]
        public Task Bind([QueryField] int SinkID, [QueryField] int SourceID)
        {
            SinkManager.Instance.BindSourceToSink(SinkID, SourceID);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // does not have to recieve a sourceId, only for multi source sinks (which do not exist in this version)
        // PATCH: Unbind a sink from a source;
        [Route(HttpVerbs.Patch, "/unbind")]
        public Task Unbind([QueryField] int SinkID, [QueryField] int? SourceID = null)
        {
            SinkManager.Instance.UnbindSourceFromSink(SinkID, SourceID);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // DELETE: delete a certain sink;
        [Route(HttpVerbs.Delete, "/delete")]
        public Task Delete([QueryField] int SinkID)
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
    }
}
