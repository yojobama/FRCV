using System.Diagnostics.Contracts;
using EmbedIO;
using EmbedIO.Routing;

namespace Server.Controllers;

public class SourceController
{
    public SourceController()
    {
        // Constructor logic here
    }

    /// API endpoint definitions

    // get all source ids
    [Route(HttpVerbs.Get, "/source/ids")]
    public Task<int[]> GetAllSourceIdsAsync()
    {
        // Logic to retrieve all source IDs
        return Task.FromResult(new int[] { });
    }

    // get parsed source by id
    [Route(HttpVerbs.Get, "/source/{sourceId}")]
    public Task<Source> GetParsedSourceByIdAsync(int sourceId)
    {
        // Logic to retrieve a parsed source by its ID
        return Task.FromResult(new Source(0, "Default", SourceType.Camera));
    }
    
    // add source
    [Route(HttpVerbs.Put, "/source/add {name} {type}")]
    public Task AddSourceAsync(string name, string type)
    {
        return Task.CompletedTask;
    }
    
    // change source type
    [Route(HttpVerbs.Put, "/source/changeType {sourceId} {type}")]
    public Task ChangeSourceTypeAsync(int sourceId, string type)
    {
        return Task.CompletedTask;
    }
    
    // change source name
    [Route(HttpVerbs.Put, "/source/changeName {sourceId} {name}")]
    public Task ChangeSourceNameAsync(int sourceId, string name)
    {
        // Logic to change the source name
        return Task.CompletedTask;
    }
    
    // upload a new video file (client to server)
    [Route(HttpVerbs.Post, "/source/uploadVideoFile")]
    public Task UploadVideoFileAsync()
    {
        // Logic to handle video file upload
        return Task.CompletedTask;
    }
    
    // upload a new image file
    [Route(HttpVerbs.Post, "/source/uploadImageFile")]
    public Task UploadImageFileAsync()
    {
        // logic to upload image file
        return Task.CompletedTask;
    }
    
    // get all camera hardware infos
    [Route(HttpVerbs.Get, "/source/camera/hardwareInfos")]
    public Task<CameraHardwareInfo[]> GetCameraHardwareInfosAsync()
    {
        // Logic to retrieve all camera hardware infos
        return Task.FromResult(new CameraHardwareInfo[] { });
    }
    
    // change camera source hardware id
    [Route(HttpVerbs.Put, "/source/camera/changeHardwareId {sourceId} {hardwareId}")]
    public Task ChangeCameraSourceHardwareIdAsync(int sourceId, string hardwareId)
    {
        // Logic to change the camera source hardware ID
        return Task.CompletedTask;
    }
    
    // delete source
    [Route(HttpVerbs.Delete, "/source/delete {sourceId}")]
    public Task DeleteSourceAsync(int sourceId)
    {
        // Logic to delete a source by its ID
        return Task.CompletedTask;
    }
}