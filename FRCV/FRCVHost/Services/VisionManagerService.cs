using Microsoft.AspNetCore.SignalR;
using System.Collections.Concurrent;

namespace FRCVHost.Services
{
    public class VisionManagerService : IVisionManagerService
    {
        private readonly Manager _manager;
        private readonly IHubContext<VisionHub> _hubContext;
        private readonly ConcurrentDictionary<int, SourceInfo> _sources = new();
        private readonly ConcurrentDictionary<int, SinkInfo> _sinks = new();

        public VisionManagerService(IHubContext<VisionHub> hubContext)
        {
            _hubContext = hubContext;
            _manager = new Manager(); // Initialize with default constructor
            StartResultPolling();
        }

        private void StartResultPolling()
        {
            // Create a timer to poll for results every 100ms
            Timer timer = new Timer(async _ =>
            {
                var results = await GetAllSinkResultsAsync();
                foreach (var result in results)
                {
                    await _hubContext.Clients.All.SendAsync("ReceiveSinkResult", result.Key, result.Value);
                }
            }, null, TimeSpan.Zero, TimeSpan.FromMilliseconds(100));
        }

        public async Task<List<CameraInfo>> GetAvailableCamerasAsync()
        {
            var cameras = _manager.enumerateAvailableCameras();
            return cameras.Select((cam, index) => new CameraInfo(index, $"Camera {index}", cam.path)).ToList();
        }

        public async Task<int> CreateCameraSourceAsync(CameraInfo info)
        {
            var hardwareInfo = new CameraHardwareInfo { path = info.Path };
            var sourceId = _manager.createCameraSource(hardwareInfo);
            _sources[sourceId] = new SourceInfo(sourceId, "Camera", "Stopped");
            return sourceId;
        }

        public async Task<int> CreateVideoFileSourceAsync(string path, int fps)
        {
            var sourceId = _manager.createVideoFileSource(path, fps);
            _sources[sourceId] = new SourceInfo(sourceId, "Video", "Stopped");
            return sourceId;
        }

        public async Task<int> CreateImageFileSourceAsync(string path)
        {
            var sourceId = _manager.createImageFileSource(path);
            _sources[sourceId] = new SourceInfo(sourceId, "Image", "Stopped");
            return sourceId;
        }

        public async Task<bool> StartSourceAsync(int sourceId)
        {
            var result = _manager.startSourceById(sourceId);
            if (result && _sources.TryGetValue(sourceId, out var source))
            {
                _sources[sourceId] = source with { Status = "Running" };
            }
            return result;
        }

        public async Task<bool> StopSourceAsync(int sourceId)
        {
            var result = _manager.stopSourceById(sourceId);
            if (result && _sources.TryGetValue(sourceId, out var source))
            {
                _sources[sourceId] = source with { Status = "Stopped" };
            }
            return result;
        }

        public async Task StartAllSourcesAsync()
        {
            _manager.startAllSources();
            foreach (var sourceId in _sources.Keys)
            {
                if (_sources.TryGetValue(sourceId, out var source))
                {
                    _sources[sourceId] = source with { Status = "Running" };
                }
            }
        }

        public async Task StopAllSourcesAsync()
        {
            _manager.stopAllSources();
            foreach (var sourceId in _sources.Keys)
            {
                if (_sources.TryGetValue(sourceId, out var source))
                {
                    _sources[sourceId] = source with { Status = "Stopped" };
                }
            }
        }

        public async Task<int> CreateApriltagSinkAsync()
        {
            var sinkId = _manager.createApriltagSink();
            _sinks[sinkId] = new SinkInfo(sinkId, "Apriltag", "Stopped", "");
            return sinkId;
        }

        public async Task<int> CreateObjectDetectionSinkAsync()
        {
            var sinkId = _manager.createObjectDetectionSink();
            _sinks[sinkId] = new SinkInfo(sinkId, "ObjectDetection", "Stopped", "");
            return sinkId;
        }

        public async Task<int> CreateRecordingSinkAsync(int sourceId)
        {
            var sinkId = _manager.createRecordingSink(sourceId);
            _sinks[sinkId] = new SinkInfo(sinkId, "Recording", "Stopped", "");
            return sinkId;
        }

        public async Task<bool> BindSourceToSinkAsync(int sourceId, int sinkId)
        {
            return _manager.bindSourceToSink(sourceId, sinkId);
        }

        public async Task<bool> UnbindSourceFromSinkAsync(int sinkId)
        {
            return _manager.unbindSourceFromSink(sinkId);
        }

        public async Task<List<SinkInfo>> GetAllSinksAsync()
        {
            return _sinks.Values.ToList();
        }

        public async Task<string> GetSinkResultAsync(int sinkId)
        {
            return _manager.getSinkResult(sinkId);
        }

        public async Task<Dictionary<int, string>> GetAllSinkResultsAsync()
        {
            var results = new Dictionary<int, string>();
            foreach (var sink in _sinks.Keys)
            {
                results[sink] = await GetSinkResultAsync(sink);
            }
            return results;
        }

        public async Task StartAllSinksAsync()
        {
            _manager.startAllSinks();
            foreach (var sinkId in _sinks.Keys)
            {
                if (_sinks.TryGetValue(sinkId, out var sink))
                {
                    _sinks[sinkId] = sink with { Status = "Running" };
                }
            }
        }

        public async Task StopAllSinksAsync()
        {
            _manager.stopAllSinks();
            foreach (var sinkId in _sinks.Keys)
            {
                if (_sinks.TryGetValue(sinkId, out var sink))
                {
                    _sinks[sinkId] = sink with { Status = "Stopped" };
                }
            }
        }

        public async Task<bool> StartSinkAsync(int sinkId)
        {
            var result = _manager.startSinkById(sinkId);
            if (result && _sinks.TryGetValue(sinkId, out var sink))
            {
                _sinks[sinkId] = sink with { Status = "Running" };
            }
            return result;
        }

        public async Task<bool> StopSinkAsync(int sinkId)
        {
            var result = _manager.stopSinkById(sinkId);
            if (result && _sinks.TryGetValue(sinkId, out var sink))
            {
                _sinks[sinkId] = sink with { Status = "Stopped" };
            }
            return result;
        }

        public async Task<int> CreateCameraCalibrationSinkAsync(int height, int width)
        {
            return _manager.createCameraCalibrationSink(height, width);
        }

        public async Task BindSourceToCalibrationSinkAsync(int sourceId)
        {
            _manager.bindSourceToCalibrationSink(sourceId);
        }

        public async Task CameraCalibrationGrabFrameAsync(int sinkId)
        {
            _manager.cameraCalibrationSinkGrabFrame(sinkId);
        }

        public async Task<string> GetCameraCalibrationResultsAsync(int sinkId)
        {
            var result = _manager.getCameraCalibrationResults(sinkId);
            return System.Text.Json.JsonSerializer.Serialize(result);
        }

        public async Task<List<SourceInfo>> GetAllSourcesAsync()
        {
            // Replace with actual implementation to fetch sources
            return await Task.FromResult(new List<SourceInfo>
            {
                new SourceInfo(1, "Camera", "Active"),
                new SourceInfo(2, "File", "Inactive")
            });
        }
    }
}