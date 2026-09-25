using Microsoft.AspNetCore.Http;
using System.IO;
using System.Text;
using System.Threading.Tasks;

namespace Server.Web
{
    // The handful of EmbedIO IHttpContext helpers the controllers actually use, re-implemented on
    // ASP.NET Core's HttpContext with the same names/signatures so the controller bodies migrate
    // unchanged - and, importantly, byte-for-byte the same wire output: EmbedIO's SendStringAsync
    // writes the encoding's preamble (so Encoding.UTF8 means a BOM, UTF8Encoding(false) means none),
    // which WebRTCSinkController.CreateOffer vs. the JSON endpoints each rely on deliberately.
    public static class HttpContextCompat
    {
        public static async Task SendStringAsync(this HttpContext context, string content, string contentType, Encoding encoding)
        {
            context.Response.ContentType = $"{contentType}; charset={encoding.WebName}";
            byte[] preamble = encoding.GetPreamble();
            byte[] body = encoding.GetBytes(content);
            context.Response.ContentLength = preamble.Length + body.Length;
            if (preamble.Length > 0) await context.Response.Body.WriteAsync(preamble, context.RequestAborted);
            await context.Response.Body.WriteAsync(body, context.RequestAborted);
        }

        public static async Task<string> GetRequestBodyAsStringAsync(this HttpContext context)
        {
            using var reader = new StreamReader(context.Request.Body, Encoding.UTF8);
            return await reader.ReadToEndAsync();
        }

        public static Stream OpenRequestStream(this HttpContext context) => context.Request.Body;
    }
}
