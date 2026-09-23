using System.Collections.Generic;
using System.Linq;

namespace Server
{
    // ROADMAP.md Phase 8a: a machine-readable description of what each node type is and how it
    // may be wired up, so the webui's graph editor can render parameter forms and reject invalid
    // connections structurally instead of hardcoding one-off knowledge about each node type (and
    // finding out a connection was invalid only from a server 500, which is the exact class of
    // bug the stereo docs currently only warn about in prose). A static manifest rather than
    // anything reflection-derived - node wiring rules (source counts, role labels, the
    // DepthFusionSink depth-attach side channel) live in C++/C# logic across several files
    // (SinkManager.BindSourceToSink/BindStereoSourcesToSink/AttachDepthFusionSource,
    // ISink::maxSources, IStereoRoleReceiver) with no single reflectable source of truth, so this
    // is hand-authored and kept in sync deliberately, the same way SinkType's own enum already is.
    public record struct NodeTypeCapability(
        string TypeName,
        string Category,              // "source" | "sink"
        string DisplayName,
        string Icon,                  // lucide-react icon name, matching the set already used across webui/src
        int MaxSources,               // 0 for a real (camera/file) source - it has nothing bound to it
        string[]? SourceRoles,        // null = a single unlabeled bind; ["left","right"] for stereo nodes
        bool IsDualRoleSink,          // can itself be bound as another sink's source (its own detection/
                                       // fusion output) - see SinkManager.DualRoleSinkTypes
        bool HasDepthAttach,          // DepthFusionSink only - a second, non-Source input via
                                       // AttachDepthFusionSource, not an ordinary bind
        bool Implemented              // false for a node type with no working creation path
    );

    public static class NodeCapabilities
    {
        // each entry's IsDualRoleSink mirrors SinkManager.DualRoleSinkTypes by hand (kept as a
        // literal duplicate rather than a shared reference so this file stays a pure,
        // dependency-free description - SinkManager pulls in the whole Manager/native call
        // surface; this doesn't need to). Keep the two lists in sync if either changes.

        public static readonly IReadOnlyList<NodeTypeCapability> Sources = new List<NodeTypeCapability>
        {
            new("Camera", "source", "Camera", "camera", 0, null, false, false, true),
            new("ImageFile", "source", "Image File", "image", 0, null, false, false, true),
            new("VideoFile", "source", "Video File", "video", 0, null, false, false, true),
            // SinkOutput is synthetic (a dual-role sink acting as its own source, see
            // SourceType's own comment in Source.cs) - not something a user creates directly, so
            // it's omitted from this list rather than described with a misleading "0 sources".
        };

        public static readonly IReadOnlyList<NodeTypeCapability> Sinks = new List<NodeTypeCapability>
        {
            new("ApriltagSink", "sink", "AprilTag Detector", "scan", 1, null, true, false, true),
            new("ObjectDetectionSink", "sink", "Object Detection", "box", 1, null, true, false, true),
            new("CameraCalibrationSink", "sink", "Camera Calibration", "grid", 1, null, true, false, true),
            new("NetworkTablesSink", "sink", "NetworkTables", "radio", 1, null, false, false, true),
            new("WebRTCSink", "sink", "WebRTC Preview", "video", 1, null, false, false, true),
            new("MjpegSink", "sink", "MJPEG Preview", "video", 1, null, false, false, true),
            new("RecordSink", "sink", "Recording", "film", 1, null, false, false, true),
            new("StereoCalibrationSink", "sink", "Stereo Calibration", "grid", 2, new[] { "left", "right" }, true, false, true),
            new("StereoDepthSink", "sink", "Stereo Depth", "layers", 2, new[] { "left", "right" }, true, false, true),
            new("DepthFusionSink", "sink", "Depth Fusion", "combine", 1, null, true, true, true),
        };

        public static NodeTypeCapability? FindSink(SinkType type) =>
            Sinks.FirstOrDefault(c => c.TypeName == type.ToString());

        public static NodeTypeCapability? FindSource(SourceType type) =>
            Sources.FirstOrDefault(c => c.TypeName == type.ToString());
    }
}
