using System;
using System.Linq;

namespace Server
{
    // ROADMAP.md Phase 8a: plain, OpenAPI-friendly response/request shapes for the handful of
    // endpoints that used to serialize a SWIG-generated type directly. Those types work (SWIG
    // already round-trips CameraCalibrationResult through System.Text.Json today, e.g.
    // CalibrationManager's own persistence), but every property getter is a live PINVOKE call
    // into the native library rather than a plain field read, and the field names are the raw
    // C++ member spelling (lowercase: fx/fy/width/height) rather than conventional C# PascalCase
    // - both are surprises a reflection-driven OpenAPI generator (see Server/OpenApi) would bake
    // straight into the generated TypeScript client. Map once at the controller boundary instead.

    public record struct CameraModeDto(int Width, int Height, double Fps, FrameFormat PixelFormat, bool IsNative)
    {
        public static CameraModeDto From(CameraMode mode) =>
            new(mode.width, mode.height, mode.fps, mode.pixelFormat, mode.isNative);

        public CameraMode ToNative()
        {
            var mode = new CameraMode
            {
                width = Width,
                height = Height,
                fps = Fps,
                pixelFormat = PixelFormat,
                isNative = IsNative
            };
            return mode;
        }
    }

    public record struct CameraHardwareInfoDto(string Name, string Path)
    {
        public static CameraHardwareInfoDto From(CameraHardwareInfo info) => new(info.name, info.path);

        public CameraHardwareInfo ToNative() => new() { name = Name, path = Path };
    }

    public record struct CameraCalibrationResultDto(
        double Fx, double Fy, double Cx, double Cy, double Rms,
        double[] DistCoeffs, int ImageWidth, int ImageHeight)
    {
        public static CameraCalibrationResultDto From(CameraCalibrationResult result) => new(
            result.fx, result.fy, result.cx, result.cy, result.rms,
            result.distCoeffs.ToArray(), result.imageWidth, result.imageHeight);

        public CameraCalibrationResult ToNative()
        {
            var distCoeffs = new VectorDouble();
            foreach (double d in DistCoeffs) distCoeffs.Add(d);
            return new CameraCalibrationResult(Fx, Fy, Cx, Cy, Rms, distCoeffs, ImageWidth, ImageHeight);
        }
    }

    public record struct StoredCalibrationDto(string CameraPath, CameraCalibrationResultDto Result, long CalibratedAtUnixMs)
    {
        // deliberately NOT touching StoredCalibration/CalibrationManager's own on-disk shape -
        // that JSON is already persisted (calibrations.json) with the raw SWIG field names, and
        // changing it would silently strand every already-saved calibration on an existing
        // deployment. This DTO only exists at the REST response boundary.
        public static StoredCalibrationDto From(StoredCalibration stored) =>
            new(stored.CameraPath, CameraCalibrationResultDto.From(stored.Result), stored.CalibratedAtUnixMs);
    }

    public record struct CalibrationStatusDto(bool HasCalibration, bool MatchesCurrentResolution, int? CalibratedWidth, int? CalibratedHeight)
    {
        public static CalibrationStatusDto From(CalibrationStatus status) =>
            new(status.HasCalibration, status.MatchesCurrentResolution, status.CalibratedWidth, status.CalibratedHeight);
    }

    public record struct StereoCalibrationResultDto(
        CameraCalibrationResultDto Left, CameraCalibrationResultDto Right,
        double[] R, double[] T, double[] E, double[] F,
        double[] R1, double[] R2, double[] P1, double[] P2, double[] Q,
        double StereoRms, double EpipolarRms, double BaselineMeters,
        double RectifiedFx, double RectifiedCx, double RectifiedCy,
        int ImageWidth, int ImageHeight,
        int RoiLeftX, int RoiLeftY, int RoiLeftW, int RoiLeftH,
        int RoiRightX, int RoiRightY, int RoiRightW, int RoiRightH)
    {
        public static StereoCalibrationResultDto From(StereoCalibrationResult r) => new(
            CameraCalibrationResultDto.From(r.left), CameraCalibrationResultDto.From(r.right),
            r.R.ToArray(), r.T.ToArray(), r.E.ToArray(), r.F.ToArray(),
            r.R1.ToArray(), r.R2.ToArray(), r.P1.ToArray(), r.P2.ToArray(), r.Q.ToArray(),
            r.stereoRms, r.epipolarRms, r.baselineMeters,
            r.rectifiedFx, r.rectifiedCx, r.rectifiedCy,
            r.imageWidth, r.imageHeight,
            r.roiLeftX, r.roiLeftY, r.roiLeftW, r.roiLeftH,
            r.roiRightX, r.roiRightY, r.roiRightW, r.roiRightH);

        private static VectorDouble Vec(double[] values)
        {
            var v = new VectorDouble();
            foreach (double d in values) v.Add(d);
            return v;
        }

        public StereoCalibrationResult ToNative() => new()
        {
            left = Left.ToNative(),
            right = Right.ToNative(),
            R = Vec(R),
            T = Vec(T),
            E = Vec(E),
            F = Vec(F),
            R1 = Vec(R1),
            R2 = Vec(R2),
            P1 = Vec(P1),
            P2 = Vec(P2),
            Q = Vec(Q),
            stereoRms = StereoRms,
            epipolarRms = EpipolarRms,
            baselineMeters = BaselineMeters,
            rectifiedFx = RectifiedFx,
            rectifiedCx = RectifiedCx,
            rectifiedCy = RectifiedCy,
            imageWidth = ImageWidth,
            imageHeight = ImageHeight,
            roiLeftX = RoiLeftX,
            roiLeftY = RoiLeftY,
            roiLeftW = RoiLeftW,
            roiLeftH = RoiLeftH,
            roiRightX = RoiRightX,
            roiRightY = RoiRightY,
            roiRightW = RoiRightW,
            roiRightH = RoiRightH
        };
    }

    public record struct StereoDepthStatsDto(double ValidFraction, double MedianDepthMeters);

    // Threads/QuadDecimate are genuinely user-adjustable (not hardcoded - see
    // ApriltagDetector's own constructor comment). QuadDecimateSupported is false for the
    // Vulkan backend (fixed 2x decimation baked into its compute pipeline), telling the
    // Inspector to hide/disable that control rather than let a user set a value that's
    // silently ignored.
    public record struct ApriltagTuningDto(int Threads, float QuadDecimate, bool QuadDecimateSupported);

    // ROADMAP.md Phase 8d: every saved snapshot/pair's detected corner points, for the
    // calibration wizard's live coverage heatmap - which region of the frame still needs more
    // checkerboard coverage. Each entry in Snapshots is one snapshot's corners flattened as
    // [x0,y0,x1,y1,...] (SWIG has no vector<vector<double>> binding - see
    // CameraCalibrator::GetSnapshotCorners' own comment).
    public record struct CalibrationCoverageDto(int FrameWidth, int FrameHeight, double[][] Snapshots);

    // NetworkTablesSink::GetConnectionStatus()/WebRTCSink::GetConnectionStatus() both return an
    // nlohmann::json object serialized to a std::string - the SWIG boundary can only express that
    // as a C# string, so the two controllers used to hand it back as Task<string> and the webui
    // had to JSON.parse() it a second time. Parse it once here into a real typed object instead.
    public record struct NetworkTablesStatusDto(bool Connected, string Identity, string RootTable, int? TeamNumber, string? ServerAddress)
    {
        public static NetworkTablesStatusDto Parse(string json)
        {
            using var doc = System.Text.Json.JsonDocument.Parse(json);
            var root = doc.RootElement;
            return new NetworkTablesStatusDto(
                root.GetProperty("connected").GetBoolean(),
                root.GetProperty("identity").GetString() ?? "",
                root.GetProperty("rootTable").GetString() ?? "",
                root.TryGetProperty("teamNumber", out var tn) ? tn.GetInt32() : null,
                root.TryGetProperty("serverAddress", out var sa) ? sa.GetString() : null);
        }
    }

    public record struct WebRtcStatusDto(bool Connected, int IceState, bool GatheringComplete)
    {
        public static WebRtcStatusDto Parse(string json)
        {
            using var doc = System.Text.Json.JsonDocument.Parse(json);
            var root = doc.RootElement;
            return new WebRtcStatusDto(
                root.GetProperty("connected").GetBoolean(),
                root.GetProperty("iceState").GetInt32(),
                root.GetProperty("gatheringComplete").GetBoolean());
        }
    }
}
