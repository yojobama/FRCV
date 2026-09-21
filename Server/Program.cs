using EmbedIO;
using EmbedIO.Files;
using EmbedIO.WebApi;
using EmbedIO.Cors;
using Server.WebSockets;
using System.Text;

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
                    // RecordingSinkController/RecordSink were deleted outright rather than left
                    // half-implemented (ROADMAP.md Phase 0's own "implement or delete" decision):
                    // the C++ side never initialized its VideoWriter, Manager::CreateRecordingSink
                    // never actually registered a sink, and the controller only ever threw
                    // NotImplementedException - nothing real to keep.
                    //
                    // Registered from RegisteredControllers.All (not one m.WithController<T>()
                    // call per type) so this list and Server/OpenApi's generated document can
                    // never drift apart - a controller reachable here is exactly a controller
                    // documented there, and vice versa.
                    foreach (var controllerType in RegisteredControllers.All)
                    {
                        m.WithController(controllerType);
                    }
                })
                // ROADMAP.md Phase 8a: the push channel replacing the webui's old polling loop -
                // see StateChannel's own comment for what it broadcasts and why.
                .WithModule(new StateChannel("/ws/state"));

            // Serves the built React WebUI (npm run build in reactproject1, copied into
            // wwwroot by Server.csproj) - registered after WithWebApi so /api/* is always
            // matched by the API module first; EmbedIO tries modules in registration order.
            // isImmutable=false: a Vite build's index.html is small and changes on every
            // rebuild, so it shouldn't be served from a stale cache.
            string wwwrootPath = Path.Combine(AppContext.BaseDirectory, "wwwroot");
            if (Directory.Exists(wwwrootPath))
            {
                // ROADMAP.md Phase 8b: react-router-dom now owns real client-side routes
                // (/graph, /sources, /match, ...) - EmbedIO's own FileModule matches by path,
                // so a direct navigation or hard refresh on any of those returns a 404 (the
                // Vite dev server's own history-mode fallback was hiding this during
                // development - only caught because it was checked against a production-style
                // build, not just `npm run dev`). HandleMappingFailed serves index.html for any
                // unmapped GET instead, the standard SPA fallback - client-side routing then
                // takes over as normal once React mounts.
                server = server.WithStaticFolder("/", wwwrootPath, false, m => m.HandleMappingFailed(async (context, info) =>
                {
                    string indexPath = Path.Combine(wwwrootPath, "index.html");
                    if (File.Exists(indexPath))
                    {
                        await context.SendStringAsync(await File.ReadAllTextAsync(indexPath), "text/html", Encoding.UTF8);
                    }
                    else
                    {
                        throw HttpException.NotFound();
                    }
                }));
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
