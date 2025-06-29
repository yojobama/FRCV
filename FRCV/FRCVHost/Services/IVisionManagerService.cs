using System.Collections.Generic;
using System.Threading.Tasks;

namespace FRCVHost.Services
{
    public record CameraInfo(int Id, string Name, string Path);
    public record SourceInfo(int Id, string Type, string Status);
    public record SinkInfo(int Id, string Type, string Status, string Result);

    public interface IVisionManagerService
    {
        // Source management
        Task<List<CameraInfo>> GetAvailableCamerasAsync();
        Task<int> CreateCameraSourceAsync(CameraInfo info);
        Task<int> CreateVideoFileSourceAsync(string path, int fps);
        Task<int> CreateImageFileSourceAsync(string path);
        Task<bool> StartSourceAsync(int sourceId);
        Task<bool> StopSourceAsync(int sourceId);
        Task StartAllSourcesAsync();
        Task StopAllSourcesAsync();
        Task<List<SourceInfo>> GetAllSourcesAsync();

        // Sink management
        Task<int> CreateApriltagSinkAsync();
        Task<int> CreateObjectDetectionSinkAsync();
        Task<int> CreateRecordingSinkAsync(int sourceId);
        Task<bool> BindSourceToSinkAsync(int sourceId, int sinkId);
        Task<bool> UnbindSourceFromSinkAsync(int sinkId);
        Task<List<SinkInfo>> GetAllSinksAsync();
        Task<string> GetSinkResultAsync(int sinkId);
        Task<Dictionary<int, string>> GetAllSinkResultsAsync();
        Task StartAllSinksAsync();
        Task StopAllSinksAsync();
        Task<bool> StartSinkAsync(int sinkId);
        Task<bool> StopSinkAsync(int sinkId);

        // Camera Calibration
        Task<int> CreateCameraCalibrationSinkAsync(int height, int width);
        Task BindSourceToCalibrationSinkAsync(int sourceId);
        Task CameraCalibrationGrabFrameAsync(int sinkId);
        Task<string> GetCameraCalibrationResultsAsync(int sinkId);
    }
}