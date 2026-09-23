using System.Text.Json;

namespace Server
{
    public class StoredCalibration
    {
        public string CameraPath { get; set; } = "";
        public CameraCalibrationResult Result { get; set; } = new CameraCalibrationResult();
        public long CalibratedAtUnixMs { get; set; }
    }

    // ROADMAP.md Phase 8/E5: a calibration is only valid at the exact resolution it was computed
    // at (see StoredCalibration's own comment on the codebase's existing lookup key), but nothing
    // previously surfaced when a camera's CURRENT mode had drifted away from that - a
    // SetCameraMode call silently left any bound ApriltagDetector's pose estimation running on
    // now-mismatched (or, worse, stale-but-still-attached) intrinsics with no visible warning.
    public record struct CalibrationStatus(bool HasCalibration, bool MatchesCurrentResolution, int? CalibratedWidth, int? CalibratedHeight);

    // Persists calibration results keyed by camera device path + resolution (a result is only
    // valid for the exact resolution it was computed at), so a calibration survives a server
    // restart and doesn't need re-doing every time a camera source is recreated.
    //
    // NOTE: auto-applying a stored result when a matching camera source is created (so a fresh
    // ApriltagDetector node picks up real distortion coefficients without a manual calibrator
    // round-trip) is not wired up yet - this only covers save/list/lookup. Follow-up work.
    public class CalibrationManager
    {
        public static CalibrationManager Instance { get; } = new CalibrationManager();

        private readonly string path = "calibrations.json";
        private readonly List<StoredCalibration> calibrations = new();

        private CalibrationManager()
        {
            Load();
        }

        private void Load()
        {
            if (!File.Exists(path)) return;
            try
            {
                var loaded = JsonSerializer.Deserialize<List<StoredCalibration>>(File.ReadAllText(path));
                if (loaded != null) calibrations.AddRange(loaded);
            }
            catch (Exception) { /* corrupt file - start empty rather than crash the server */ }
        }

        private void Save()
        {
            File.WriteAllText(path, JsonSerializer.Serialize(calibrations, new JsonSerializerOptions { WriteIndented = true }));
        }

        // resolves the camera path a calibrator sink is bound to (if any) and persists the
        // result under it; a calibrator with no bound camera source (or bound to a non-camera
        // source, e.g. a video file used for bench testing) is not persisted - there is nothing
        // to key it by that would still mean anything after a restart
        public void SaveResult(int calibratorSinkId, CameraCalibrationResult result)
        {
            var sink = SinkManager.Instance.GetSinkById(calibratorSinkId);
            string? cameraPath = sink?.Source?.CameraHardwareInfo?.path;
            if (cameraPath == null) return;

            calibrations.RemoveAll(c => c.CameraPath == cameraPath && c.Result.imageWidth == result.imageWidth && c.Result.imageHeight == result.imageHeight);
            calibrations.Add(new StoredCalibration
            {
                CameraPath = cameraPath,
                Result = result,
                CalibratedAtUnixMs = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
            });
            Save();
        }

        public StoredCalibration? GetLatest(string cameraPath, int width, int height)
        {
            return calibrations
                .Where(c => c.CameraPath == cameraPath && c.Result.imageWidth == width && c.Result.imageHeight == height)
                .OrderByDescending(c => c.CalibratedAtUnixMs)
                .FirstOrDefault();
        }

        public List<StoredCalibration> GetAll() => calibrations;

        // Not GetLatest(cameraPath, width, height) - this deliberately ignores the current
        // resolution when looking a calibration up, then compares it against the camera's actual
        // current mode itself, so it can tell "never calibrated" apart from "calibrated, but not
        // at this resolution" rather than treating both as one "no match" case the way an
        // exact-key lookup would.
        public CalibrationStatus GetCalibrationStatus(int sourceId)
        {
            Source? source = SourceManager.Instance.GetSourceById(sourceId);
            string? cameraPath = source?.CameraHardwareInfo?.path;
            if (cameraPath == null) return new CalibrationStatus(false, false, null, null);

            StoredCalibration? latest = calibrations
                .Where(c => c.CameraPath == cameraPath)
                .OrderByDescending(c => c.CalibratedAtUnixMs)
                .FirstOrDefault();
            if (latest == null) return new CalibrationStatus(false, false, null, null);

            CameraMode currentMode = ManagerWrapper.Instance.GetCameraCurrentMode(sourceId);
            bool matches = latest.Result.imageWidth == currentMode.width && latest.Result.imageHeight == currentMode.height;
            return new CalibrationStatus(true, matches, latest.Result.imageWidth, latest.Result.imageHeight);
        }
    }
}
