using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System.ComponentModel;
using System.Xml.Linq;
using SIPSorcery.Net;
using System.Text.Json;

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
        return Task.FromResult(ManagerWrapper.Instance.GetAllSinks().ToArray());
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
        int sinkId = SinkManager.Instance.AddSink(name, type);
        DB.Instance.Save(); // Save changes to the database
        return Task.FromResult(sinkId);
    }

    // change sink name
    [Route(HttpVerbs.Put, "/sink/changeName")]
    public Task ChangeSinkNameAsync([QueryField] int sinkId, [QueryField] string name)
    {
        try
        {
            SinkManager.Instance.SetSinkName(sinkId, name);
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
        DB.Instance.Save(); // Save changes to the database
        return Task.CompletedTask;
    }
    
    // unbind sink from source (specify source id)
    [Route(HttpVerbs.Put, "/sink/unbind")]
    public Task UnbindSinkFromSourceAsync([QueryField] int sinkId, [QueryField] int sourceId)
    {
        SinkManager.Instance.UnbindSourceFromSink(sinkId, sourceId);
        DB.Instance.Save(); // Save changes to the database
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

    // WebRTC Video Streaming Endpoints

    // Start WebRTC video stream for a sink
    [Route(HttpVerbs.Post, "/sink/webrtc/start")]
    public async Task<object> StartWebRTCStreamAsync([QueryField] int sinkId, [QueryField] string connectionId)
    {
        try
        {
            Console.WriteLine($"Starting WebRTC stream for sink {sinkId}, connection {connectionId}");
            
            var offer = await WebRTCStreamingService.Instance.StartStreamAsync(sinkId, connectionId);
            
            return new
            {
                success = true,
                sinkId = sinkId,
                connectionId = connectionId,
                offer = new
                {
                    type = offer.type.ToString().ToLower(),
                    sdp = offer.sdp
                }
            };
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error starting WebRTC stream for sink {sinkId}: {ex.Message}");
            return new
            {
                success = false,
                error = ex.Message
            };
        }
    }

    // Handle WebRTC answer from client
    [Route(HttpVerbs.Post, "/sink/webrtc/answer")]
    public async Task<object> HandleWebRTCAnswerAsync()
    {
        try
        {
            var requestBody = await HttpContext.GetRequestBodyAsStringAsync();
            var request = JsonSerializer.Deserialize<WebRTCAnswerRequest>(requestBody);
            
            if (request == null)
            {
                return new { success = false, error = "Invalid request body" };
            }

            Console.WriteLine($"Handling WebRTC answer for sink {request.SinkId}, connection {request.ConnectionId}");
            
            var answer = new RTCSessionDescriptionInit
            {
                type = RTCSdpType.answer,
                sdp = request.Answer.Sdp
            };

            await WebRTCStreamingService.Instance.HandleAnswerAsync(request.SinkId, request.ConnectionId, answer);
            
            return new { success = true };
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error handling WebRTC answer: {ex.Message}");
            return new { success = false, error = ex.Message };
        }
    }

    // Handle ICE candidate from client
    [Route(HttpVerbs.Post, "/sink/webrtc/ice")]
    public async Task<object> HandleIceCandidateAsync()
    {
        try
        {
            var requestBody = await HttpContext.GetRequestBodyAsStringAsync();
            var request = JsonSerializer.Deserialize<IceCandidateRequest>(requestBody);
            
            if (request == null)
            {
                return new { success = false, error = "Invalid request body" };
            }

            Console.WriteLine($"Handling ICE candidate for sink {request.SinkId}, connection {request.ConnectionId}");
            
            var candidate = new RTCIceCandidateInit
            {
                candidate = request.Candidate.Candidate,
                sdpMid = request.Candidate.SdpMid,
                sdpMLineIndex = request.Candidate.SdpMLineIndex ?? 0
            };

            await WebRTCStreamingService.Instance.HandleIceCandidateAsync(request.SinkId, request.ConnectionId, candidate);
            
            return new { success = true };
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error handling ICE candidate: {ex.Message}");
            return new { success = false, error = ex.Message };
        }
    }

    // Stop WebRTC video stream for a sink
    [Route(HttpVerbs.Post, "/sink/webrtc/stop")]
    public Task<object> StopWebRTCStreamAsync([QueryField] int sinkId)
    {
        try
        {
            Console.WriteLine($"Stopping WebRTC stream for sink {sinkId}");
            
            WebRTCStreamingService.Instance.StopStream(sinkId);
            
            return Task.FromResult<object>(new { success = true, sinkId = sinkId });
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error stopping WebRTC stream for sink {sinkId}: {ex.Message}");
            return Task.FromResult<object>(new { success = false, error = ex.Message });
        }
    }

    // Get WebRTC stream status for a sink
    [Route(HttpVerbs.Get, "/sink/webrtc/status")]
    public Task<object> GetWebRTCStreamStatusAsync([QueryField] int sinkId)
    {
        try
        {
            bool isStreaming = WebRTCStreamingService.Instance.IsStreaming(sinkId);
            
            return Task.FromResult<object>(new 
            { 
                success = true, 
                sinkId = sinkId, 
                isStreaming = isStreaming 
            });
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error getting WebRTC stream status for sink {sinkId}: {ex.Message}");
            return Task.FromResult<object>(new { success = false, error = ex.Message });
        }
    }

    // Enable/disable sink preview
    [Route(HttpVerbs.Put, "/sink/preview/enable")]
    public Task<object> EnableSinkPreviewAsync([QueryField] int sinkId)
    {
        try
        {
            ManagerWrapper.Instance.EnableSinkPreview(sinkId);
            return Task.FromResult<object>(new { success = true, sinkId = sinkId, previewEnabled = true });
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error enabling sink preview for sink {sinkId}: {ex.Message}");
            return Task.FromResult<object>(new { success = false, error = ex.Message });
        }
    }

    [Route(HttpVerbs.Put, "/sink/preview/disable")]
    public Task<object> DisableSinkPreviewAsync([QueryField] int sinkId)
    {
        try
        {
            ManagerWrapper.Instance.DisableSinkPreview(sinkId);
            return Task.FromResult<object>(new { success = true, sinkId = sinkId, previewEnabled = false });
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error disabling sink preview for sink {sinkId}: {ex.Message}");
            return Task.FromResult<object>(new { success = false, error = ex.Message });
        }
    }

    // Get preview image data (for testing/debugging)
    [Route(HttpVerbs.Get, "/sink/preview/image")]
    public Task<object> GetPreviewImageAsync([QueryField] int sinkId)
    {
        try
        {
            var image8U = ManagerWrapper.Instance.GetPreviewImage(sinkId);
            
            return Task.FromResult<object>(new 
            { 
                success = true, 
                sinkId = sinkId, 
                width = image8U.width, 
                height = image8U.height,
                stride = image8U.stride
            });
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error getting preview image for sink {sinkId}: {ex.Message}");
            return Task.FromResult<object>(new { success = false, error = ex.Message });
        }
    }
}

// Data transfer objects for WebRTC signaling
public class WebRTCAnswerRequest
{
    public int SinkId { get; set; }
    public string ConnectionId { get; set; } = string.Empty;
    public SdpData Answer { get; set; } = new SdpData();
}

public class IceCandidateRequest
{
    public int SinkId { get; set; }
    public string ConnectionId { get; set; } = string.Empty;
    public CandidateData Candidate { get; set; } = new CandidateData();
}

public class SdpData
{
    public string Type { get; set; } = string.Empty;
    public string Sdp { get; set; } = string.Empty;
}

public class CandidateData
{
    public string Candidate { get; set; } = string.Empty;
    public string? SdpMid { get; set; }
    public ushort? SdpMLineIndex { get; set; }
}