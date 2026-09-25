using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http.Features;
using Microsoft.AspNetCore.Mvc.ApplicationParts;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.FileProviders;
using Microsoft.Extensions.Hosting;
using Server.HttpModules;
using Server.Web;
using Server.WebSockets;
using System.Text.Json.Serialization;

namespace Server
{
    internal class Program
    {
        // "*" (not "localhost") so the WebUI and API are reachable from other machines on the
        // network - this is what actually makes "running the WebUI on the Pi" useful, since nobody
        // browses to the Orange Pi's own localhost. ASPNETCORE_URLS overrides it (standard Kestrel
        // behaviour) for anyone who needs a different bind.
        //
        // 5800: FRC field networks only pass ports 5800-5810 for team use (the old 8175 would be
        // dropped on a real field). Same port PhotonVision uses; the two can't run at once anyway
        // (lumenvision.service Conflicts=photonvision.service). WebRTC media takes 5801-5809 (see
        // LumenCore/WebRTCSink.cpp) and 5810 is the roboRIO's NT4 server.
        private const string DefaultUrl = "http://*:5800";

        private static WebApplication CreateWebServer(string[] args)
        {
            string wwwrootPath = Path.Combine(AppContext.BaseDirectory, "wwwroot");
            bool hasWebUi = Directory.Exists(wwwrootPath);

            var builder = WebApplication.CreateBuilder(new WebApplicationOptions
            {
                Args = args,
                ContentRootPath = AppContext.BaseDirectory,
                WebRootPath = hasWebUi ? wwwrootPath : null,
            });

            // proper sd_notify/SIGTERM integration under lumenvision.service (Type=simple still
            // works too) - replaces the old "block on stdin unless redirected" workaround, which
            // existed only because Console.ReadLine() hit EOF instantly under systemd.
            builder.Host.UseSystemd();

            if (Environment.GetEnvironmentVariable("ASPNETCORE_URLS") == null)
                builder.WebHost.UseUrls(DefaultUrl);

            builder.WebHost.ConfigureKestrel(o =>
            {
                // model (.onnx/.rknn) and video uploads routinely exceed Kestrel's 30 MB default;
                // this is a LAN coprocessor, not an internet-facing server
                o.Limits.MaxRequestBodySize = null;
                // a few controllers read request bodies through a synchronous StreamReader, as
                // they did under EmbedIO
                o.AllowSynchronousIO = true;
            });
            builder.Services.Configure<FormOptions>(o =>
            {
                o.MultipartBodyLengthLimit = long.MaxValue;
                o.ValueLengthLimit = int.MaxValue;
            });

            builder.Services.AddCors(o => o.AddDefaultPolicy(p => p.AllowAnyOrigin().AllowAnyHeader().AllowAnyMethod()));

            builder.Services
                .AddControllers(o =>
                {
                    o.Conventions.Add(new ApiPrefixConvention());
                    o.Filters.Add(new RejectUnparseableParametersFilter());
                    // <Nullable>enable</Nullable> would otherwise make MVC treat every non-nullable
                    // string parameter as implicitly [Required] - an omitted optional name must
                    // keep binding null, as it did under EmbedIO, not become a 400
                    o.SuppressImplicitRequiredAttributeForNonNullableReferenceTypes = true;
                    // a Task<string> action must stay a JSON string on the wire ("libx264", with
                    // quotes, application/json) exactly as under EmbedIO - ASP.NET's
                    // StringOutputFormatter would otherwise send it as bare text/plain, breaking
                    // every client that res.json()s e.g. /webrtcSink/preferredEncoder. The
                    // endpoints that deliberately send raw text (SDP, pre-rendered JSON) write
                    // their bodies directly via SendStringAsync instead.
                    o.OutputFormatters.RemoveType<Microsoft.AspNetCore.Mvc.Formatters.StringOutputFormatter>();
                })
                .ConfigureApplicationPartManager(m =>
                {
                    // Registered from RegisteredControllers.All (not ASP.NET's default assembly
                    // scan) so this list and Server/OpenApi's generated document can never drift
                    // apart - a controller reachable here is exactly a controller documented
                    // there, and vice versa.
                    foreach (var provider in m.FeatureProviders.OfType<Microsoft.AspNetCore.Mvc.Controllers.ControllerFeatureProvider>().ToList())
                        m.FeatureProviders.Remove(provider);
                    m.FeatureProviders.Add(new RegisteredControllerFeatureProvider());
                })
                .AddJsonOptions(o =>
                {
                    // PascalCase on the wire, exactly as under EmbedIO (and as /ws/state and
                    // data.json already were) - ASP.NET's own default would be camelCase, which
                    // would break every webui read like `s.Sink.Id`. Enums stay numeric (no
                    // JsonStringEnumConverter), matching OpenApiGenerator's documented schemas.
                    o.JsonSerializerOptions.PropertyNamingPolicy = null;
                    // a NaN/Infinity (e.g. a mean over zero samples) must not 500 the endpoint
                    o.JsonSerializerOptions.NumberHandling = JsonNumberHandling.AllowNamedFloatingPointLiterals;
                });

            builder.Services.AddHostedService<StateChannelBroadcaster>();

            var app = builder.Build();

            app.UseMiddleware<ApiExceptionMiddleware>();

            if (hasWebUi)
            {
                // Serves the built React WebUI (npm run build in webui, copied into wwwroot by
                // Server.csproj). isImmutable-style caching deliberately not enabled: a Vite
                // build's index.html is small and changes on every rebuild.
                app.UseDefaultFiles();
                app.UseStaticFiles();
            }
            else
            {
                Console.WriteLine($"WARNING: wwwroot not found at '{wwwrootPath}' - WebUI will not be served (API is still available under /api)");
            }

            // Explicitly AFTER static files: WebApplication otherwise inserts UseRouting at the very
            // start of the pipeline, so the SPA fallback endpoint below would already be selected
            // for /assets/index-*.js before UseStaticFiles ran - and the static file middleware
            // deliberately stands aside once an endpoint has been chosen, serving index.html as
            // the "JavaScript" (confirmed the hard way: the webui's own bundle came back text/html).
            app.UseRouting();
            app.UseCors();
            app.UseWebSockets(new WebSocketOptions { KeepAliveInterval = TimeSpan.FromSeconds(15) });

            // ROADMAP.md Phase 8a: the push channel replacing the webui's old polling loop - see
            // StateChannel's own comment for what it broadcasts and why.
            app.Map("/ws/state", StateChannel.Instance.HandleAsync);
            // Outside /api, same reasoning as /ws/state - a long-lived response, not a REST call.
            app.MapGet("/stream/mjpeg", MjpegStreamModule.HandleAsync);
            app.MapControllers();

            if (hasWebUi)
            {
                // ROADMAP.md Phase 8b: react-router-dom owns real client-side routes (/graph,
                // /sources, /match, ...) - a direct navigation or hard refresh on any of those must
                // get index.html (the standard SPA fallback), then client-side routing takes over
                // once React mounts. /api/* is excluded so an unknown API route is a real 404 for
                // API clients, not a 200 HTML page.
                app.MapFallbackToFile("{*path:regex(^(?!api(/|$)).*$)}", "index.html",
                    new StaticFileOptions { FileProvider = new PhysicalFileProvider(wwwrootPath) });
            }

            return app;
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

            Console.WriteLine("Starting HTTP server...");
            var app = CreateWebServer(args);
            // blocks until SIGTERM/SIGINT (systemd stop/restart, or Ctrl+C in a terminal)
            app.Run();
        }
    }
}
