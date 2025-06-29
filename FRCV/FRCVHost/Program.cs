using FRCVHost.Components;
using MudBlazor.Services;
using FRCVHost.Services;

namespace FRCVHost
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var builder = WebApplication.CreateBuilder(args);

            // Add services to the container.
            builder.Services.AddRazorComponents()
                .AddInteractiveServerComponents();

            // Add MudBlazor
            builder.Services.AddMudServices();

            // Add SignalR
            builder.Services.AddSignalR();

            // Add our Vision Manager service as singleton
            builder.Services.AddSingleton<IVisionManagerService, VisionManagerService>();
            builder.Services.AddSingleton<IWebRTCService, WebRTCService>();

            var app = builder.Build();

            // Configure the HTTP request pipeline.
            if (!app.Environment.IsDevelopment())
            {
                app.UseExceptionHandler("/Error");
                app.UseHsts();
            }

            app.UseHttpsRedirection();
            app.UseStaticFiles();
            app.UseAntiforgery();

            // Map SignalR hub
            app.MapHub<VisionHub>("/visionHub");

            app.MapRazorComponents<App>()
                .AddInteractiveServerRenderMode();

            app.Run();
        }
    }
}
