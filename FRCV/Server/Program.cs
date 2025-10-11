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
            // initializing the resource monitor
            LinuxResourceMonitor.Instance.StartMonitoring();
            Thread.Sleep(3000); // giving the monitor time to stabilize
            
            // initializing the computer vision system (I know, a very specific description)
            Console.WriteLine("Loading database...");
            DB.Instance.Load();
            DB.Instance.Verify();

            // initializing the web server
            Console.WriteLine("Starting HTTP server...");
            var server = CreateWebServer("http://localhost:8175");
            server.Start();

            Console.WriteLine("Server is running. Press Enter to exit.");
            Console.ReadLine();
        }
    }
}
