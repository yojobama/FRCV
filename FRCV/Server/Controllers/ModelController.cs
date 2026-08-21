using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using HttpMultipartParser;

namespace Server.Controllers
{
    // upload/list/delete for YOLOv8/v11 ONNX detection models. The variant is a plain form
    // field (the WebUI's model-family dropdown), not inferred from the file - both variants
    // share the same ONNX head shape, so there is nothing in the file itself to detect it from.
    internal class ModelController : WebApiController
    {
        // POST multipart/form-data: fields "name", "variant" (0=YOLOv8, 1=YOLOv11),
        // "inputSize", "confThreshold", "nmsThreshold"; files "model" (.onnx, required) and
        // "labels" (one class name per line, optional)
        [Route(HttpVerbs.Post, "/model/upload")]
        public Task<int> Upload()
        {
            var parser = MultipartFormDataParser.Parse(HttpContext.OpenRequestStream());

            string name = parser.GetParameterValue("name");
            var variant = (YoloVariant)int.Parse(parser.GetParameterValue("variant"));
            int inputSize = int.TryParse(parser.GetParameterValue("inputSize"), out var s) ? s : 640;
            float confThreshold = float.TryParse(parser.GetParameterValue("confThreshold"), out var c) ? c : 0.25f;
            float nmsThreshold = float.TryParse(parser.GetParameterValue("nmsThreshold"), out var n) ? n : 0.45f;

            var modelFile = parser.Files.FirstOrDefault(f => f.Name == "model");
            if (modelFile == null) throw HttpException.BadRequest("missing required 'model' file");
            var labelsFile = parser.Files.FirstOrDefault(f => f.Name == "labels");

            var model = ModelManager.Instance.AddModel(
                name, modelFile.FileName, modelFile.Data,
                labelsFile?.FileName, labelsFile?.Data,
                variant, inputSize, confThreshold, nmsThreshold);

            return Task.FromResult(model.Id);
        }

        [Route(HttpVerbs.Get, "/model/getAll")]
        public Task<List<Model>> GetAll()
        {
            return Task.FromResult(ModelManager.Instance.GetAllModels());
        }

        [Route(HttpVerbs.Delete, "/model/delete")]
        public Task Delete([QueryField] int id)
        {
            ModelManager.Instance.DeleteModel(id);
            return Task.CompletedTask;
        }
    }
}
