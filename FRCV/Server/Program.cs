using EmbedIO;
using EmbedIO.WebApi;
using Server.Controllers;

namespace Server
{
    internal class Program
    {
        private static WebServer CreateWebServer(string url)
        {
            var server = new WebServer(o => o
                    .WithUrlPrefix(url)
                    .WithMode(HttpListenerMode.EmbedIO))
                .WithLocalSessionManager()
                .WithWebApi("/api", m =>
                {
                    m.WithController<SourceController>();
                    m.WithController<SinkController>();
                });

            // Optional: Listen for state changes
            server.StateChanged += (s, e) =>
                Console.WriteLine($"WebServer New State - {e.NewState}");

            return server;
        }

        static void Main(string[] args)
        {
            Console.WriteLine("Hello, World!");
            
            var server = CreateWebServer("http://localhost:8175");
            
            server.Start();
            

            Console.ReadLine();
        }
    }
}
