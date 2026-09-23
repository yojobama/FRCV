using EmbedIO;
using System;
using System.Text;
using System.Threading.Tasks;

namespace Server.HttpModules
{
    // ROADMAP.md Phase 8/E7: serves MjpegSink's latest frame as a long-lived
    // multipart/x-mixed-replace HTTP response - a raw EmbedIO module, not a WebApiController
    // action, since a normal controller method is expected to return once with a complete
    // response; this deliberately never returns until the client disconnects or the server shuts
    // down. Same "long-lived response needs a custom module, not a controller" reasoning
    // StateChannel.cs already documents for /ws/state - that one is a WebSocketModule (a
    // bidirectional, message-framed channel); this is a plain WebModuleBase (a single ordinary
    // HTTP response whose body just never ends), since a browser's own <img> tag has no idea
    // what a WebSocket is.
    public class MjpegStreamModule : WebModuleBase
    {
        // ~10fps cap - MjpegSink itself only re-encodes as fast as its bound source actually
        // produces frames; this just bounds how often this loop re-checks for a new one, not the
        // real frame rate. Skips re-sending an unchanged frame (see the loop below) rather than
        // resending stale bytes just because the interval elapsed with nothing new processed yet.
        private static readonly TimeSpan PollInterval = TimeSpan.FromMilliseconds(100);
        private const string Boundary = "lumenvision-mjpeg-frame";

        public MjpegStreamModule(string baseRoute) : base(baseRoute)
        {
        }

        public override bool IsFinalHandler => true;

        protected override async Task OnRequestAsync(IHttpContext context)
        {
            if (!int.TryParse(context.Request.QueryString["SinkID"], out int sinkId))
            {
                throw HttpException.BadRequest("SinkID query parameter is required");
            }

            context.Response.ContentType = $"multipart/x-mixed-replace; boundary={Boundary}";
            context.Response.KeepAlive = true;
            context.Response.SendChunked = true;

            string lastFrameBase64 = "";
            while (!context.CancellationToken.IsCancellationRequested)
            {
                string frameBase64 = SinkManager.Instance.GetMjpegFrameBase64(sinkId);
                if (frameBase64.Length > 0 && frameBase64 != lastFrameBase64)
                {
                    byte[] jpegBytes = Convert.FromBase64String(frameBase64);
                    byte[] header = Encoding.ASCII.GetBytes(
                        $"--{Boundary}\r\nContent-Type: image/jpeg\r\nContent-Length: {jpegBytes.Length}\r\n\r\n");
                    byte[] footer = Encoding.ASCII.GetBytes("\r\n");

                    await context.Response.OutputStream.WriteAsync(header, context.CancellationToken);
                    await context.Response.OutputStream.WriteAsync(jpegBytes, context.CancellationToken);
                    await context.Response.OutputStream.WriteAsync(footer, context.CancellationToken);
                    await context.Response.OutputStream.FlushAsync(context.CancellationToken);

                    lastFrameBase64 = frameBase64;
                }

                try
                {
                    await Task.Delay(PollInterval, context.CancellationToken);
                }
                catch (TaskCanceledException)
                {
                    break; // client disconnected (or server shutting down) - not a real error
                }
            }
        }
    }
}
