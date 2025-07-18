using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.ComponentModel;
using System.Xml.Linq;

namespace Server.Controllers;

public class SinkController : WebApiController
{
    private List<UDPTransmiter> udpTransmiters = new List<UDPTransmiter>();
    
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
    [Route(HttpVerbs.Get, "/sink/getSinkById")]
    public Task<Sink> GetParsedSinkByIdAsync([QueryField] int sinkId)
    {
        return Task.FromResult(SinkManager.Instance.GetSinkById(sinkId));
    }
    
    // enable sink
    [Route(HttpVerbs.Put, "/sink/enable")]
    public Task EnableSinkAsync([QueryField] int sinkId)
    {
        SinkManager.Instance.EnableSinkById(sinkId);
        return Task.CompletedTask;
    }
    
    // disable sink
    [Route(HttpVerbs.Put, "/sink/disable")]
    public Task DisableSinkAsync([QueryField] int sinkId)
    {
        SinkManager.Instance.DisableSinkById(sinkId);
        return Task.CompletedTask;
    }

    // add sink
    [Route(HttpVerbs.Post, "/sink/add")]
    public Task<int> AddSinkAsync([QueryField] string name, [QueryField] string type)
    {
        return Task.FromResult(SinkManager.Instance.AddSink(name, type));
    }

    // change sink name
    [Route(HttpVerbs.Put, "/sink/changeName")]
    public Task ChangeSinkNameAsync([QueryField] int sinkId, [QueryField] string name)
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
    [Route(HttpVerbs.Delete, "/sink/delete")]
    public Task DeleteSinkAsync([QueryField] int sinkId)
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
    [Route(HttpVerbs.Put, "/sink/bind")]
    public Task BindSinkToSourceAsync([QueryField] int sourceId, [QueryField] int sinkId)
    {
        SinkManager.Instance.BindSourceToSink(sinkId, sourceId);
        return Task.CompletedTask;
    }
    
    // unbind sink from source (specify source id)
    [Route(HttpVerbs.Put, "/sink/unbind")]
    public Task UnbindSinkFromSourceAsync([QueryField] int sinkId, [QueryField] int sourceId)
    {
        SinkManager.Instance.UnbindSourceFromSink(sinkId, sourceId);
        return Task.CompletedTask;
    }
    
    // start udp transmissions to my address
    [Route(HttpVerbs.Put, "/sink/startUdp")]
    public Task StartUdpTransmissionsAsync()
    {
        SinkManager.Instance.EnableManagerThread();
        var transmitter = new UDPTransmiter(SinkManager.Instance.createResultChannel(), HttpContext.Request.RemoteEndPoint.Address.ToString(), 12345);
        transmitter.Enable();
        udpTransmiters.Add(transmitter);
        return Task.CompletedTask;
    }
    
    // stop udp transmissions to my address
    [Route(HttpVerbs.Put, "/sink/stopUdp")]
    public Task StopUdpTransmissionsAsync()
    {
        // Logic to start UDP transmissions
        string clientAddress = HttpContext.Request.RemoteEndPoint.Address.ToString();
        var transmitter = udpTransmiters.FirstOrDefault(x => x.HostName == clientAddress);
        if (transmitter != null)
        {
            transmitter.Disable();
            udpTransmiters.Remove(transmitter);
        }
        if(udpTransmiters.Count == 0)
        {
            SinkManager.Instance.DisableManagerThread();
        }
        return Task.CompletedTask;
    }
}