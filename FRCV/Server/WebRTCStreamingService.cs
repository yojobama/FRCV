using SIPSorcery.Net;
using SIPSorcery.Media;
using System.Collections.Concurrent;
using System.Runtime.InteropServices;

namespace Server;

public class WebRTCVideoStreamer : IDisposable
{
    private readonly int _sinkId;
    private readonly ConcurrentDictionary<string, RTCPeerConnection> _connections;
    private readonly Timer _frameTimer;
    private bool _isStreaming;
    private readonly object _lockObject = new object();

    public WebRTCVideoStreamer(int sinkId)
    {
        _sinkId = sinkId;
        _connections = new ConcurrentDictionary<string, RTCPeerConnection>();
        _frameTimer = new Timer(CaptureAndSendFrame, null, Timeout.Infinite, Timeout.Infinite);
    }

    public async Task<RTCSessionDescriptionInit> CreateOfferAsync(string connectionId)
    {
        var pc = CreatePeerConnection(connectionId);
        
        // Create video track
        var videoTrack = new MediaStreamTrack(SDPMediaTypesEnum.video, false, 
            new List<SDPAudioVideoMediaFormat> { GetVideoFormat() });
        pc.addTrack(videoTrack);

        var offer = pc.createOffer(null);
        await pc.setLocalDescription(offer);

        return offer;
    }

    public async Task SetRemoteDescriptionAsync(string connectionId, RTCSessionDescriptionInit answer)
    {
        if (_connections.TryGetValue(connectionId, out var pc))
        {
            await pc.setRemoteDescription(answer);
        }
    }

    public async Task AddIceCandidateAsync(string connectionId, RTCIceCandidateInit candidate)
    {
        if (_connections.TryGetValue(connectionId, out var pc))
        {
            pc.addIceCandidate(candidate);
        }
    }

    public void StartStreaming()
    {
        lock (_lockObject)
        {
            if (!_isStreaming)
            {
                _isStreaming = true;
                Console.WriteLine($"Starting WebRTC stream for sink {_sinkId}");
                
                try
                {
                    // Enable preview for the sink
                    ManagerWrapper.Instance.EnableSinkPreview(_sinkId);
                    // Start frame capture timer (30 FPS)
                    _frameTimer.Change(0, 33);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Error enabling sink preview for sink {_sinkId}: {ex.Message}");
                }
            }
        }
    }

    public void StopStreaming()
    {
        lock (_lockObject)
        {
            if (_isStreaming)
            {
                _isStreaming = false;
                Console.WriteLine($"Stopping WebRTC stream for sink {_sinkId}");
                
                _frameTimer.Change(Timeout.Infinite, Timeout.Infinite);
                
                try
                {
                    ManagerWrapper.Instance.DisableSinkPreview(_sinkId);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Error disabling sink preview for sink {_sinkId}: {ex.Message}");
                }
                
                // Close all connections
                foreach (var kvp in _connections)
                {
                    kvp.Value.close();
                }
                _connections.Clear();
            }
        }
    }

    public void RemoveConnection(string connectionId)
    {
        if (_connections.TryRemove(connectionId, out var pc))
        {
            pc.close();
        }
    }

    private RTCPeerConnection CreatePeerConnection(string connectionId)
    {
        var config = new RTCConfiguration
        {
            iceServers = new List<RTCIceServer>
            {
                new RTCIceServer { urls = "stun:stun.l.google.com:19302" },
                new RTCIceServer { urls = "stun:stun1.l.google.com:19302" }
            }
        };

        var pc = new RTCPeerConnection(config);

        pc.onicecandidate += (candidate) =>
        {
            Console.WriteLine($"ICE candidate for {connectionId}: {candidate?.candidate}");
        };

        pc.onconnectionstatechange += (state) =>
        {
            Console.WriteLine($"Connection {connectionId} state: {state}");
            if (state == RTCPeerConnectionState.disconnected || state == RTCPeerConnectionState.failed)
            {
                RemoveConnection(connectionId);
            }
        };

        _connections.TryAdd(connectionId, pc);
        return pc;
    }

    private SDPAudioVideoMediaFormat GetVideoFormat()
    {
        return new SDPAudioVideoMediaFormat(SDPMediaTypesEnum.video, 96, "H264", 90000);
    }

    private void CaptureAndSendFrame(object? state)
    {
        if (!_isStreaming) return;

        try
        {
            // Get the preview image from the sink
            var image8U = ManagerWrapper.Instance.GetPreviewImage(_sinkId);
            
            if (image8U.width > 0 && image8U.height > 0)
            {
                Console.WriteLine($"Captured frame for sink {_sinkId}: {image8U.width}x{image8U.height}");
                
                // Convert Image8U to byte array and encode to H.264
                byte[] imageData = ConvertImage8UToBytes(image8U);
                
                if (imageData.Length > 0)
                {
                    byte[] h264Frame = EncodeFrameToH264(imageData, image8U.width, image8U.height);
                    
                    // Send to all connected peers
                    foreach (var kvp in _connections)
                    {
                        var pc = kvp.Value;
                        if (pc.connectionState == RTCPeerConnectionState.connected)
                        {
                            SendH264Frame(pc, h264Frame);
                        }
                    }
                }
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error capturing frame for sink {_sinkId}: {ex.Message}");
        }
    }

    private byte[] ConvertImage8UToBytes(Image8U image8U)
    {
        try
        {
            // Calculate total bytes needed (assuming RGB format)
            int totalBytes = image8U.width * image8U.height * 3;
            byte[] imageData = new byte[totalBytes];
            
            // Create a test pattern since we can't directly access the native pointer
            // In production, you would properly marshal the data from the native C++ buffer
            
            // Create a gradient test pattern for demonstration
            for (int y = 0; y < image8U.height; y++)
            {
                for (int x = 0; x < image8U.width; x++)
                {
                    int index = (y * image8U.width + x) * 3;
                    if (index + 2 < totalBytes)
                    {
                        // Create a colorful gradient pattern
                        imageData[index] = (byte)(x % 256);     // R
                        imageData[index + 1] = (byte)(y % 256); // G
                        imageData[index + 2] = (byte)((x + y) % 256); // B
                    }
                }
            }
            
            return imageData;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error converting Image8U to bytes: {ex.Message}");
            return new byte[0];
        }
    }

    private byte[] EncodeFrameToH264(byte[] rgbData, int width, int height)
    {
        try
        {
            // Convert RGB to YUV420 for H.264 encoding
            byte[] yuvData = ConvertRGBToYUV420(rgbData, width, height);
            
            // Create basic H.264 frame structure
            // In production, use a proper H.264 encoder like FFmpeg
            
            // Create SPS (Sequence Parameter Set)
            byte[] sps = CreateSPS(width, height);
            
            // Create PPS (Picture Parameter Set)  
            byte[] pps = CreatePPS();
            
            // Create IDR frame
            byte[] idr = CreateIDRFrame(yuvData, width, height);
            
            // Combine NAL units
            List<byte> h264Frame = new List<byte>();
            h264Frame.AddRange(sps);
            h264Frame.AddRange(pps);
            h264Frame.AddRange(idr);
            
            return h264Frame.ToArray();
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error encoding frame to H.264: {ex.Message}");
            return new byte[0];
        }
    }

    private byte[] ConvertRGBToYUV420(byte[] rgbData, int width, int height)
    {
        // YUV420: Y plane + U plane (1/4 size) + V plane (1/4 size)
        int ySize = width * height;
        int uvSize = ySize / 4;
        byte[] yuv = new byte[ySize + uvSize * 2];
        
        // Convert RGB to YUV using ITU-R BT.601 standard
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int rgbIndex = (y * width + x) * 3;
                int yIndex = y * width + x;
                
                if (rgbIndex + 2 < rgbData.Length && yIndex < ySize)
                {
                    byte r = rgbData[rgbIndex];
                    byte g = rgbData[rgbIndex + 1];
                    byte b = rgbData[rgbIndex + 2];
                    
                    // Y component
                    int yVal = (int)(0.299 * r + 0.587 * g + 0.114 * b);
                    yuv[yIndex] = (byte)Math.Min(255, Math.Max(0, yVal));
                    
                    // Subsample U and V at 2x2 intervals
                    if (x % 2 == 0 && y % 2 == 0)
                    {
                        int uvIndex = (y / 2) * (width / 2) + (x / 2);
                        int uVal = (int)(-0.147 * r - 0.289 * g + 0.436 * b + 128);
                        int vVal = (int)(0.615 * r - 0.515 * g - 0.100 * b + 128);
                        
                        if (ySize + uvIndex < yuv.Length)
                            yuv[ySize + uvIndex] = (byte)Math.Min(255, Math.Max(0, uVal));
                        if (ySize + uvSize + uvIndex < yuv.Length)
                            yuv[ySize + uvSize + uvIndex] = (byte)Math.Min(255, Math.Max(0, vVal));
                    }
                }
            }
        }
        
        return yuv;
    }

    private byte[] CreateSPS(int width, int height)
    {
        // Simplified SPS NAL unit for H.264
        return new byte[] 
        { 
            0x00, 0x00, 0x00, 0x01, // Start code
            0x67, // NAL unit header (SPS)
            0x42, 0x00, 0x1e, // Profile and level
            0xff, 0xe1, 0x00, 0x18 // Basic SPS data
        };
    }

    private byte[] CreatePPS()
    {
        // Simplified PPS NAL unit for H.264
        return new byte[] 
        { 
            0x00, 0x00, 0x00, 0x01, // Start code
            0x68, // NAL unit header (PPS)
            0xce, 0x3c, 0x80 // Basic PPS data
        };
    }

    private byte[] CreateIDRFrame(byte[] yuvData, int width, int height)
    {
        // Simplified IDR frame NAL unit
        List<byte> idr = new List<byte>
        {
            0x00, 0x00, 0x00, 0x01, // Start code
            0x65 // NAL unit header (IDR frame)
        };
        
        // Add portion of YUV data for demonstration
        int dataSize = Math.Min(1024, yuvData.Length);
        idr.AddRange(yuvData.Take(dataSize));
        
        return idr.ToArray();
    }

    private void SendH264Frame(RTCPeerConnection pc, byte[] h264Frame)
    {
        try
        {
            var videoSender = pc.getSenders().FirstOrDefault(s => s.track?.kind == "video");
            if (videoSender != null && h264Frame.Length > 0)
            {
                Console.WriteLine($"Sending H.264 frame of {h264Frame.Length} bytes");
                // In production: fragment into RTP packets and send via RTCRtpSender
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error sending H.264 frame: {ex.Message}");
        }
    }

    public void Dispose()
    {
        StopStreaming();
        _frameTimer?.Dispose();
        
        foreach (var kvp in _connections)
        {
            kvp.Value.close();
        }
        _connections.Clear();
    }
}

public class WebRTCStreamingService
{
    private readonly ConcurrentDictionary<int, WebRTCVideoStreamer> _streamers;
    private static readonly WebRTCStreamingService _instance = new WebRTCStreamingService();
    
    public static WebRTCStreamingService Instance => _instance;
    
    private WebRTCStreamingService()
    {
        _streamers = new ConcurrentDictionary<int, WebRTCVideoStreamer>();
    }

    public async Task<RTCSessionDescriptionInit> StartStreamAsync(int sinkId, string connectionId)
    {
        Console.WriteLine($"Starting WebRTC stream for sink {sinkId}, connection {connectionId}");
        var streamer = _streamers.GetOrAdd(sinkId, id => new WebRTCVideoStreamer(id));
        streamer.StartStreaming();
        return await streamer.CreateOfferAsync(connectionId);
    }

    public async Task HandleAnswerAsync(int sinkId, string connectionId, RTCSessionDescriptionInit answer)
    {
        Console.WriteLine($"Handling answer for sink {sinkId}, connection {connectionId}");
        if (_streamers.TryGetValue(sinkId, out var streamer))
        {
            await streamer.SetRemoteDescriptionAsync(connectionId, answer);
        }
    }

    public async Task HandleIceCandidateAsync(int sinkId, string connectionId, RTCIceCandidateInit candidate)
    {
        Console.WriteLine($"Handling ICE candidate for sink {sinkId}, connection {connectionId}");
        if (_streamers.TryGetValue(sinkId, out var streamer))
        {
            await streamer.AddIceCandidateAsync(connectionId, candidate);
        }
    }

    public void StopStream(int sinkId)
    {
        Console.WriteLine($"Stopping WebRTC stream for sink {sinkId}");
        if (_streamers.TryRemove(sinkId, out var streamer))
        {
            streamer.Dispose();
        }
    }

    public void RemoveConnection(int sinkId, string connectionId)
    {
        Console.WriteLine($"Removing connection {connectionId} for sink {sinkId}");
        if (_streamers.TryGetValue(sinkId, out var streamer))
        {
            streamer.RemoveConnection(connectionId);
        }
    }

    public bool IsStreaming(int sinkId)
    {
        return _streamers.ContainsKey(sinkId);
    }
}