using EmbedIO;
using EmbedIO.WebApi;
using EmbedIO.Cors;
using Server.Controllers;
using Server.Controllers.sources;
using Server.Controllers.sinks;

namespace Server
{
    internal class Program
    {
        private static WebServer CreateWebServer(string url)
        {
            var server = new WebServer(o => o
                    .WithUrlPrefix(url)
                    .WithMode(HttpListenerMode.EmbedIO))
                .WithCors("*", "*", "*")
                .WithWebApi("/api", m =>
                {
                    // sinks
                    // TODO: Implement the other sinks and add them here
                    m.WithController<SinkController>();
                    m.WithController<ApriltagSinkController>();
                    // sources
                    m.WithController<SourceController>();
                    m.WithController<ImageFileSourceController>();
                    m.WithController<VideoFileSourceController>();
                    m.WithController<CameraSourceController>();
                    // others
                    m.WithController<UDPController>();
                    m.WithController<DeviceController>();
                });

            // Optional: Listen for state changes
            server.StateChanged += (s, e) =>
                Console.WriteLine($"WebServer New State - {e.NewState}");

            return server;
        }

        static void Main(string[] args)
        {
            Console.WriteLine("Loading database...");
            DB.Instance.Load();
            DB.Instance.Verify();

            Console.WriteLine("Starting HTTP server...");
            var server = CreateWebServer("http://localhost:8175");
            server.Start();

            Console.WriteLine("Server is running. Press Enter to exit.");
            Console.ReadLine();
        }
    }
}
