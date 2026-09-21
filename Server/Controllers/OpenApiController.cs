using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using Server.OpenApi;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers
{
    // ROADMAP.md Phase 8a: serves the reflection-generated OpenAPI document. `openapi-typescript`
    // (webui build step) points at this to regenerate the TypeScript client; also useful directly
    // (Postman/Swagger UI import) since nothing else describes this server's routes today.
    internal class OpenApiController : WebApiController
    {
        // Written as a raw string body rather than returned as a plain object - EmbedIO's default
        // response serializer is Swan's own reflection-based Json.Serialize, not System.Text.Json,
        // and it cannot resolve System.Text.Json.Nodes.JsonObject's own indexer property (throws
        // "An item with the same key has already been added. Key: (Item, Item)" trying to build
        // its property cache - confirmed the hard way). The document is built with System.Text.Json
        // (JsonNode/JsonObject) specifically for its cleaner JSON-tree API, so it's serialized to a
        // string with System.Text.Json directly here, matching how WebRTCSinkController.CreateOffer
        // already bypasses Swan for its own content-type reasons.
        // UTF8Encoding(false): plain Encoding.UTF8 writes a BOM preamble, which most JSON
        // parsers (including Python's, used to verify this endpoint) reject outright.
        private static readonly Encoding Utf8NoBom = new UTF8Encoding(false);

        [Route(HttpVerbs.Get, "/openapi.json")]
        public async Task GetOpenApiDocument()
        {
            string json = OpenApiGenerator.Generate(RegisteredControllers.All).ToJsonString();
            await HttpContext.SendStringAsync(json, "application/json", Utf8NoBom);
        }
    }
}
