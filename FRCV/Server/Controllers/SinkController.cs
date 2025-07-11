using EmbedIO;
using EmbedIO.Routing;
using System.ComponentModel;
using System.Xml.Linq;

namespace Server.Controllers;

public class SinkController
{
    public SinkController()
    {
        
    }

    /// API endpoint definitions

    // get all sink ids
    [Route(HttpVerbs.Get, "/sink/ids")]
    public Task<int[]> GetAllSinkIdsAsync()
    {
        // Logic to retrieve all sink IDs
        return Task.FromResult(ManagerWrapper.Instance.getAllSinks().ToArray());
    }
    
    // get parsed sink by id
    [Route(HttpVerbs.Get, "/sink/{sinkId}")]
    public Task<Sink> GetParsedSinkByIdAsync(int sinkId)
    {
        return Task.FromResult(SinkManager.Instance.GetSinkById(sinkId));
    }
    
    // enable sink
    [Route(HttpVerbs.Put, "/sink/enable {sinkId}")]
    public Task EnableSinkAsync(int sinkId)
    {
        SinkManager.Instance.EnableSinkById(sinkId);
        return Task.CompletedTask;
    }
    
    // disable sink
    [Route(HttpVerbs.Put, "/sink/disable {sinkId}")]
    public Task DisableSinkAsync(int sinkId)
    {
        SinkManager.Instance.DisableSinkById(sinkId);
        return Task.CompletedTask;
    }
    
    // add sink
    [Route(HttpVerbs.Post, "/sink/add {name} {type}")]
    public Task AddSinkAsync(string name, string type)
    {
        SinkManager.Instance.AddSink(name, type);
        return Task.CompletedTask;
    }

    // change sink name
    [Route(HttpVerbs.Put, "/sink/changeName {sinkId} {name}")]
    public Task ChangeSinkNameAsync(int sinkId, string name)
    {
        try
        {
            SinkManager.Instance.ChangeSinkName(sinkId, name);
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.ToString());
            return Task.FromException(ex);
        }
        return Task.CompletedTask;
    }
    
    // delete sink
    [Route(HttpVerbs.Delete, "/sink/delete {sinkId}")]
    public Task DeleteSinkAsync(int sinkId)
    {
        try
        {
            SinkManager.Instance.DeleteSink(sinkId);
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.ToString());
            return Task.FromException(ex);
        }
        return Task.CompletedTask;
    }

    // bind sink to source (specify source id and sink id)
    [Route(HttpVerbs.Put, "/sink/bind {sourceId} {sinkId}")]
    public Task BindSinkToSourceAsync(int sourceId, int sinkId)
    {
        ManagerWrapper.Instance.bindSourceToSink(sourceId, sinkId);
        return Task.CompletedTask;
    }
    
    // unbind sink from source (specify source id)
    [Route(HttpVerbs.Put, "/sink/unbind {sinkId} {sourceId}")]
    public Task UnbindSinkFromSourceAsync(int sinkId, int sourceId)
    {
        SinkManager.Instance.UnbindSourceFromSink(sinkId, sourceId);
        return Task.CompletedTask;
    }
    
    // start udp transmissions to my address
    [Route(HttpVerbs.Put, "/sink/startUdp")]
    public Task StartUdpTransmissionsAsync(string ipAddress, int port)
    {
        
        return Task.CompletedTask;
    }
    
    // stop udp transmissions to my address
    [Route(HttpVerbs.Put, "/sink/stopUdp")]
    public Task StopUdpTransmissionsAsync()
    {
        // Logic to start UDP transmissions
        return Task.CompletedTask;
    }
}