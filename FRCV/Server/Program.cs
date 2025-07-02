using EmbedIO;
using EmbedIO.WebApi;

namespace Server
{
    internal class Program
    {
        private static WebServer createWebServer(string url)
        {
            var server = new WebServer(o => o
                    .WithUrlPrefix(url)
                    .WithMode(HttpListenerMode.EmbedIO))
                // First, we will configure our web server by adding Modules.
                .WithLocalSessionManager();
            // Listen for state changes.

            return server;
        }

        static void Main(string[] args)
        {
            Console.WriteLine("Hello, World!");
        }
    }
}
