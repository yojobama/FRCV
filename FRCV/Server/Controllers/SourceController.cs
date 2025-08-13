using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using HttpMultipartParser;
using System.Diagnostics.Contracts;
using System.Net.Http;
using System.Text.Json;

namespace Server.Controllers;

public class SourceController : WebApiController
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
        // Logic to retrieve all source IDs from the C++ Manager (single source of truth)
        return Task.FromResult(ManagerWrapper.Instance.GetAllSources().ToArray());
    }

    // get parsed source by id
    [Route(HttpVerbs.Get, "/source")]
    public Task<Source?> GetParsedSourceByIdAsync([QueryField] int sourceId)
    {
        // Return the object directly so EmbedIO serializes it as JSON (avoid double-serializing to string)
        var source = SourceManager.Instance.GetSourceById(sourceId);
        if (source == null)
        {
            HttpContext.Response.StatusCode = 404;
        }
        return Task.FromResult(source);
    }

    // change source name
    [Route(HttpVerbs.Put, "/source/changeName")]
    public Task ChangeSourceNameAsync([QueryField] int sourceId, [QueryField] string name)
    {
        SourceManager.Instance.ChangeSourceName(sourceId, name);
        return Task.CompletedTask;
    }

    // create camera source
    // TODO: fix me, CameraHardwareInfo is null
    [Route(HttpVerbs.Post, "/source/createCameraSource")]
    public Task<int> CreateCameraSource([JsonData] CameraHardwareInfo hardwareInfo)
    {
        int sourceId = SourceManager.Instance.InitializeCameraSource(hardwareInfo);
        return Task.FromResult(sourceId);
    }
    
    // upload a new video file (client to server)
    [Route(HttpVerbs.Post, "/source/createVideoFileSource")]
    public Task<int> CreateVideoFileSourceAsync([QueryField] int fps)
    {
        // Logic to handle video file upload
        var parser = MultipartFormDataParser.Parse(HttpContext.OpenRequestStream());

        var file = parser.Files.FirstOrDefault();

        if (file != null)
        {
            string fileName = file.FileName;
            Stream fileStream = file.Data;

            Directory.CreateDirectory("videos");

            using (var output = File.Create(Path.Combine("videos", fileName)))
            {
                fileStream.CopyTo(output);
                return Task.FromResult(SourceManager.Instance.InitializeVideoFileSource(Path.Combine("videos", fileName), fps, Path.GetFileNameWithoutExtension(fileName)));
            }
        }

        return Task.FromResult(-1);
    }
    
    // upload a new image file
    [Route(HttpVerbs.Post, "/source/createImageFileSource")]
    public Task<int> CreateImageFileSourceAsync()
    {
        // logic to upload image file
        var parser = MultipartFormDataParser.Parse(HttpContext.OpenRequestStream());

        var file = parser.Files.FirstOrDefault();

        if (file != null)
        {
            string fileName = file.FileName;
            Stream fileStream = file.Data;

            Directory.CreateDirectory("images");

            using (var output = File.Create(Path.Combine("images", fileName)))
            {
                fileStream.CopyTo(output);
                return Task.FromResult(SourceManager.Instance.initializeImageFileSource(Path.Combine("images", fileName), Path.GetFileNameWithoutExtension(fileName)));
            }
        }
        return Task.FromResult(-1);
    }
    
    // get all camera hardware infos
    [Route(HttpVerbs.Get, "/source/camera/hardwareInfos")]
    public Task<CameraHardwareInfo[]> GetCameraHardwareInfosAsync()
    {
        // Logic to retrieve all camera hardware infos
        return Task.FromResult(ManagerWrapper.Instance.EnumerateAvailableCameras().ToArray());
    }   
    
    // delete source
    [Route(HttpVerbs.Delete, "/source/delete")]
    public Task DeleteSourceAsync([QueryField] int sourceId)
    {
        // Logic to delete a source by its ID
        return Task.CompletedTask;
    }
}