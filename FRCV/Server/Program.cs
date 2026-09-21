using EmbedIO;
using EmbedIO.Files;
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
                    m.WithController<SinkController>();
                    m.WithController<ApriltagSinkController>();
                    m.WithController<CameraCalibrationSinkController>();
                    m.WithController<NetworkTablesSinkController>();
                    m.WithController<ObjectDetectionSinkController>();
                    m.WithController<WebRTCSinkController>();
                    m.WithController<StereoCalibrationSinkController>();
                    m.WithController<StereoDepthSinkController>();
                    m.WithController<DepthFusionSinkController>();
                    // RecordingSinkController is intentionally not registered yet: recording
                    // sink creation is still a bare `throw new NotImplementedException()`.
                    // sources
                    m.WithController<SourceController>();
                    m.WithController<ImageFileSourceController>();
                    m.WithController<VideoFileSourceController>();
                    m.WithController<CameraSourceController>();
                    // models
                    m.WithController<ModelController>();
                    // others
                    m.WithController<UDPController>();
                    m.WithController<DeviceController>();
                });

            // Serves the built React WebUI (npm run build in reactproject1, copied into
            // wwwroot by Server.csproj) - registered after WithWebApi so /api/* is always
            // matched by the API module first; EmbedIO tries modules in registration order.
            // isImmutable=false: a Vite build's index.html is small and changes on every
            // rebuild, so it shouldn't be served from a stale cache.
            string wwwrootPath = Path.Combine(AppContext.BaseDirectory, "wwwroot");
            if (Directory.Exists(wwwrootPath))
            {
                server = server.WithStaticFolder("/", wwwrootPath, false);
            }
            else
            {
                Console.WriteLine($"WARNING: wwwroot not found at '{wwwrootPath}' - WebUI will not be served (API is still available under /api)");
            }

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
            // "*" (not "localhost") so the WebUI and API are reachable from other machines on
            // the network - this is what actually makes "running the WebUI on the Pi" useful,
            // since nobody browses to the Orange Pi's own localhost.
            var server = CreateWebServer("http://*:8175");
            server.Start();

            Console.WriteLine("Server is running. Press Enter to exit.");
            Console.ReadLine();
        }
    }
}
