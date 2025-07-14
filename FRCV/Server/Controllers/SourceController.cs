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
        // Logic to retrieve all source IDs
        return Task.FromResult(SourceManager.Instance.GetAllSourceIds());
    }

    // get parsed source by id
    [Route(HttpVerbs.Get, "/source")]
    public Task<string> GetParsedSourceByIdAsync([QueryField] int sourceId)
    {
        Source source = SourceManager.Instance.GetSourceById(sourceId);
        // Logic to retrieve a parsed source by its ID
        return Task.FromResult(JsonSerializer.Serialize(source));
    }

    // change source name
    [Route(HttpVerbs.Put, "/source/changeName")]
    public Task ChangeSourceNameAsync([QueryField] int sourceId, [QueryField] string name)
    {
        
        // Logic to change the source name
        return Task.CompletedTask;
    }

    // TODO: implement this fu,nction
    [Route(HttpVerbs.Post, "/source/createCameraSource")]
    public Task CreateCameraSource()
    {
        return Task.CompletedTask;
    }
    
    // upload a new video file (client to server)
    [Route(HttpVerbs.Post, "/source/createVideoFileSource")]
    public Task CreateVideoFileSourceAsync(IHttpContext context, [QueryField] int fps)
    {
        // Logic to handle video file upload
        var parser = MultipartFormDataParser.Parse(context.OpenRequestStream());

        foreach (var file in parser.Files)
        {
            string fileName = file.FileName;
            Stream fileStream = file.Data;

            using (var output = File.Create(Path.Combine("videos", fileName)))
            {
                fileStream.CopyTo(output);
                SourceManager.Instance.InitializeVideoFileSource(Path.Combine("videos", fileName), fps, Path.GetFileNameWithoutExtension(fileName));
            }

        }

        return Task.CompletedTask;
    }
    
    // upload a new image file
    [Route(HttpVerbs.Post, "/source/createImageFileSource")]
    public Task CreateImageFileSourceAsync(IHttpContext context)
    {
        // logic to upload image file
        var parser = MultipartFormDataParser.Parse(context.OpenRequestStream());

        foreach (var file in parser.Files)
        {
            string fileName = file.FileName;
            Stream fileStream = file.Data;

            using (var output = File.Create(Path.Combine("images", fileName)))
            {
                fileStream.CopyTo(output);
                SourceManager.Instance.initializeImageFileSource(Path.Combine("images", fileName), Path.GetFileNameWithoutExtension(fileName));
            }
        }

        return Task.CompletedTask;
    }
    
    // get all camera hardware infos
    [Route(HttpVerbs.Get, "/source/camera/hardwareInfos")]
    public Task<CameraHardwareInfo[]> GetCameraHardwareInfosAsync()
    {
        // Logic to retrieve all camera hardware infos
        return Task.FromResult(new CameraHardwareInfo[] { });
    }
    
    // delete source
    [Route(HttpVerbs.Delete, "/source/delete")]
    public Task DeleteSourceAsync([QueryField] int sourceId)
    {
        // Logic to delete a source by its ID
        return Task.CompletedTask;
    }
}