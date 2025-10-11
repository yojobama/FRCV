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
        // GET: Get a sink's active status by id;
        [Route(HttpVerbs.Get, "/sink/getStatus")]
        public Task<bool> GetStatus([QueryField] int SinkID)
        {
            bool status = SinkManager.Instance.IsSinkRunning(SinkID);
            return Task.FromResult(false);
        }

        // PATCH: Enable/Disable a sink;
        [Route(HttpVerbs.Patch, "/sink/toggle")]
        public Task Toggle([QueryField] int SinkID, [QueryField] bool Enabled)
        {
            if (!Enabled) SinkManager.Instance.DisableSinkById(SinkID);
            else SinkManager.Instance.EnableSinkById(SinkID);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // PATCH: Change the name of a sink;
        [Route(HttpVerbs.Patch, "/sink/rename")]
        public Task Rename([QueryField] int SinkID, [QueryField] string NewName)
        {
            SinkManager.Instance.SetSinkName(SinkID, NewName);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // PATCH: Bind a sink to a source;
        [Route(HttpVerbs.Patch, "/sink/bind")]
        public Task Bind([QueryField] int SinkID, [QueryField] int SourceID)
        {
            SinkManager.Instance.BindSourceToSink(SinkID, SourceID);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // does not have to recieve a sourceId, only for multi source sinks (which do not exist in this version)
        // PATCH: Unbind a sink from a source;
        [Route(HttpVerbs.Patch, "/sink/unbind")]
        public Task Unbind([QueryField] int SinkID, [QueryField] int? SourceID = null)
        {
            SinkManager.Instance.UnbindSourceFromSink(SinkID, SourceID);
            DB.Instance.Save(); // Save changes to the database
            return Task.CompletedTask;
        }

        // DELETE: delete a certain sink;
        [Route(HttpVerbs.Delete, "/sink/delete")]
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

        // GET: get all sinks;
        [Route(HttpVerbs.Get, "/sink/getAll")]
        public Task<List<Sink>> GetAll()
        {
            List<Sink> sinks = new List<Sink>();
            foreach (var sink in SinkManager.Instance.getAllSinkIds())
            {
                sinks.Add(SinkManager.Instance.GetSinkById(sink));
            }
            return Task.FromResult(sinks);
        }
    }
}
