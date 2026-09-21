using System;
using Server.Controllers;
using Server.Controllers.sinks;
using Server.Controllers.sources;

namespace Server
{
    // The single source of truth for which WebApiController types are actually reachable.
    // EmbedIO controllers are opt-in - a class existing doesn't mean it's registered (confirmed
    // the hard way, ROADMAP.md Phase 0: three stereo controllers existed, fully implemented,
    // completely unreachable, for want of one line each in Program.cs). Program.cs registers
    // every type in this list; Server/OpenApi/OpenApiGenerator.cs documents exactly the same
    // list - so the generated API doc can never describe a route that isn't actually wired up,
    // or omit one that is.
    public static class RegisteredControllers
    {
        public static readonly Type[] All =
        {
            // sinks
            typeof(SinkController),
            typeof(ApriltagSinkController),
            typeof(CameraCalibrationSinkController),
            typeof(NetworkTablesSinkController),
            typeof(ObjectDetectionSinkController),
            typeof(WebRTCSinkController),
            typeof(StereoCalibrationSinkController),
            typeof(StereoDepthSinkController),
            typeof(DepthFusionSinkController),
            // sources
            typeof(SourceController),
            typeof(ImageFileSourceController),
            typeof(VideoFileSourceController),
            typeof(CameraSourceController),
            typeof(PipelineProfileController),
            // models
            typeof(ModelController),
            // others
            typeof(UDPController),
            typeof(DeviceController),
            typeof(CapabilitiesController),
            typeof(OpenApiController),
        };
    }
}
