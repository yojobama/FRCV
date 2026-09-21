using System.Collections.Generic;

namespace Server
{
    // ROADMAP.md Phase 7: pipeline profiles. PhotonVision's pipelineIndex has no direct meaning
    // against a node graph - a camera Source here can already feed several Sinks at once (e.g.
    // an AprilTag detector and a WebRTC preview bound to the same camera simultaneously), so
    // "the pipeline" can't mean "the whole graph" the way it does for a single fixed
    // detect-and-publish chain. What DOES map cleanly onto PhotonVision's per-camera "N
    // alternate processing configs, switch which one runs" model is the one detection sink
    // (ApriltagSink or ObjectDetectionSink) bound to a camera - everything else feeding off that
    // same camera (WebRTC preview, driver mode) is orthogonal and stays untouched across a
    // switch, matching how PhotonVision's own driver mode is independent of pipelineIndex too.
    //
    // A profile therefore belongs to a Source (see Source.Profiles/ActiveProfileIndex/
    // ActiveDetectionSinkId) and captures everything needed to (re)create that one detection
    // sink from scratch: SourceManager.ActivateProfile deletes whatever is currently running at
    // the source's ActiveDetectionSinkId and recreates it at the SAME id from this profile's
    // settings - see that method's own comment for why the id must be preserved (downstream
    // sinks like a NetworkTablesSink or WebRTCSink are bound to the detection sink's own id as
    // their source, and Manager::DeleteSink natively unbinds them, so it re-binds them itself
    // afterwards rather than leaving them dangling).
    public enum DetectionSinkKind
    {
        ApriltagSink,
        ObjectDetectionSink
    }

    public class PipelineProfile
    {
        // stable ordinal within this Source, 0-based and assigned once at creation (never
        // reused after a delete) - this is what a robot program's pipelineIndex actually
        // refers to, so it must not shift when an earlier profile is removed.
        public int Index { get; set; }
        public string Name { get; set; }
        public DetectionSinkKind Kind { get; set; }

        // --- ApriltagSink settings (Kind == ApriltagSink) ---
        public double? TagSize { get; set; }
        // a CameraCalibrationSink id to pull intrinsics from at activation time (looked up live
        // via GetCameraCalibrationResult, not snapshotted here - a re-run calibration should be
        // picked up the next time this profile activates without having to re-save it)
        public int? CalibratorSinkId { get; set; }
        public ApriltagBackendKind? Backend { get; set; }
        public int FrameWidth { get; set; }
        public int FrameHeight { get; set; }
        // path to this profile's OWN copy of a WPILib field-layout JSON (see
        // SourceController's /source/profiles/fieldLayout) - deliberately profile-scoped, not
        // sink-id-scoped like the older single-sink /apriltagSink/fieldLayout endpoint: two
        // profiles on the same source share the same ActiveDetectionSinkId slot when each is
        // active, so keying the file by sink id would let one profile's upload silently
        // overwrite another's.
        public string? FieldLayoutPath { get; set; }
        public bool DriverMode { get; set; }

        // --- ObjectDetectionSink settings (Kind == ObjectDetectionSink) ---
        // a ModelManager-registered model id - mirrors AddObjectDetectionSink's own modelId
        // parameter, so profile creation reuses the exact same model registry as manual sink
        // creation rather than inventing a second way to reference a model.
        public int? ModelId { get; set; }

        public PipelineProfile()
        {
            Name = string.Empty;
        }
    }
}
