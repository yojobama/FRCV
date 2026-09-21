using Swan;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Channels;
using System.Threading.Tasks;

namespace Server
{
    public class SinkManager
    {
        public static SinkManager Instance { get; } = new SinkManager();

        private string currentResults;
        private Thread thread;
        private List<Channel<string>> channels;
        private List<Sink> sinks;
        private bool isRunning;

        private SinkManager()
        {
            thread = new Thread(ThreadProc);

            sinks = new List<Sink>();
            channels = new List<Channel<string>>();
            isRunning = false;
        }

        // Direct pass-through to the native, per-sink result JSON (fixed to actually work back
        // in phase 2 - it used to return nullptr/GetStatus()'s hardcoded ""). GetResults() below
        // is a SEPARATE, older mechanism (a background thread + Channel<string> fan-out) that is
        // never started anywhere in this codebase (EnableManagerThread() has no caller) and has
        // its own busy-wait bug (ThreadProc's while(isRunning) loop has no sleep) - left alone
        // for now rather than half-fixed, since nothing currently depends on it.
        public string GetResult(int sinkId) => ManagerWrapper.Instance.GetSinkResult(sinkId);
        public string GetAllResults() => ManagerWrapper.Instance.GetAllSinkResults();

        public string GetResults()
        {
            return currentResults;
        }

        private void ThreadProc()
        {
            // This method will run in a separate thread to manage sinks
            while (isRunning)
            {
                updateResults();
                foreach (var channel in channels)
                {
                    channel.Writer.TryWrite(currentResults);
                }
                // Logic to manage sinks, e.g., checking for new data, processing it, etc.
                // This could involve reading from channels and updating sinks accordingly.
            }
        }

        public Sink GetSinkById(int id)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == id)
                {
                    return sink;
                }
            }

            return null; // or throw an exception if preferred
        }

        public int[] getAllSinkIds()
        {
            List<int> ids = new List<int>();
            foreach (var sink in sinks)
            {
                ids.Add(sink.Id);
            }

            return ids.ToArray();
        }

        public List<Sink> GetAllSinks()
        {
            return sinks;
        }

        public void SetSinkName(int sinkId, string dstName)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == sinkId)
                {
                    sink.Name = dstName;
                    DB.Instance.Save(); // Save changes to the database
                    break;
                }
            }
        }

        public void DeleteSink(int sinkId)
        {
            ManagerWrapper.Instance.DeleteSink(sinkId);
            sinks.RemoveAll(sink => sink.Id == sinkId);
            DB.Instance.Save(); // Save changes to the database
        }

        public int AddSink(string name, string type, int? id = null)
        {
            // Normalize type to avoid case / typo issues
            type = (type ?? string.Empty).Trim().ToLowerInvariant();

            if (id.HasValue)
            {
                switch (type)
                {
                    // "apriltagsink" is what DB.Load() actually passes (SinkType.ApriltagSink.
                    // ToString(), lowercased) - it was missing here entirely (the "ApriltagSink"
                    // label above never matches anything post-ToLowerInvariant()), which meant
                    // every plain ApriltagSink silently failed to come back after a restart -
                    // confirmed the hard way while verifying ROADMAP.md Phase 7's pipeline
                    // profiles, which reconstruct correctly regardless since ActivateProfile
                    // creates its own sink directly rather than going through this switch, but a
                    // profile-less ApriltagSink had no such path. "apriltag"/"apritlag" kept for
                    // whatever REST callers already pass the short form.
                    case "apriltagsink":
                    case "apriltag":
                    case "apritlag": // backward-compatibility for misspelling
                        id = ManagerWrapper.Instance.CreateApriltagDetector(id.Value);
                        sinks.Add(new Sink(id.Value, name, SinkType.ApriltagSink));
                        break;
                    case "objectdetectionsink":
                    case "ObjectDetectionSink":
                        id = ManagerWrapper.Instance.CreateObjectDetectionSink(ObjectDetectionProvider.ONNX, id.Value);// TODO: Add logic for selecting acceleration type (ONNX with REP, or Rknn)
                        sinks.Add(new Sink(id.Value, name, SinkType.ObjectDetectionSink));
                        break;
                    case "cameracalibrationsink":
                    case "cameracalibration":
                        id = ManagerWrapper.Instance.CreateCameraCalibrator(id.Value);
                        sinks.Add(new Sink(id.Value, name, SinkType.CameraCalibrationSink));
                        break;
                    case "stereocalibrationsink":
                        id = ManagerWrapper.Instance.CreateStereoCalibrator(id.Value);
                        sinks.Add(new Sink(id.Value, name, SinkType.StereoCalibrationSink));
                        break;
                    case "depthfusionsink":
                        id = ManagerWrapper.Instance.CreateDepthFusionNode(id.Value);
                        sinks.Add(new Sink(id.Value, name, SinkType.DepthFusionSink));
                        break;
                    // StereoDepthSink is deliberately NOT restorable through this generic path -
                    // same gap as WebRTCSink/NetworkTablesSink above: it needs a backend,
                    // calibration result and depth range that this signature has no room for.
                    // DB.Load() re-creating it as a no-op (id stays unset) matches those sinks'
                    // existing behavior rather than introducing a new one.
                }
                return id.GetValueOrDefault(-1);
            }
            else
            {
                switch (type)
                {
                    case "apriltagsink":
                    case "apriltag":
                    case "apritlag":
                        id = ManagerWrapper.Instance.CreateApriltagDetector();
                        sinks.Add(new Sink(id.Value, name, SinkType.ApriltagSink));
                        break;
                    case "objectdetectionsink":
                        id = ManagerWrapper.Instance.CreateObjectDetectionSink(ObjectDetectionProvider.ONNX);
                        sinks.Add(new Sink(id.Value, name, SinkType.ObjectDetectionSink));
                        break;
                    case "cameracalibrationsink":
                    case "cameracalibration":
                        id = ManagerWrapper.Instance.CreateCameraCalibrator();
                        sinks.Add(new Sink(id.Value, name, SinkType.CameraCalibrationSink));
                        break;
                    case "stereocalibrationsink":
                        id = ManagerWrapper.Instance.CreateStereoCalibrator();
                        sinks.Add(new Sink(id.Value, name, SinkType.StereoCalibrationSink));
                        break;
                    case "depthfusionsink":
                        id = ManagerWrapper.Instance.CreateDepthFusionNode();
                        sinks.Add(new Sink(id.Value, name, SinkType.DepthFusionSink));
                        break;
                }

                DB.Instance.Save(); // Save changes to the database
                return id.GetValueOrDefault(-1);
            }
        }

        public Channel<string> createResultChannel()
        {
            Channel<string> channel = Channel.CreateUnbounded<string>();

            channels.Add(channel);

            return channel;
        }

        // creates a CameraCalibrationSink with an explicit board configuration (checkerboard or
        // ChArUco); AddSink(name, "cameracalibrationsink") still exists for the default 6x9/25mm
        // checkerboard
        public int AddCameraCalibrationSinkWithBoard(string name, CalibrationBoardType boardType, int rows, int cols,
            float squareSizeMeters, float markerSizeMeters = 0.018f, int arucoDictionaryId = 10)
        {
            int id = ManagerWrapper.Instance.CreateCameraCalibrator(boardType, rows, cols, squareSizeMeters, markerSizeMeters, arucoDictionaryId);
            sinks.Add(new Sink(id, name, SinkType.CameraCalibrationSink));
            DB.Instance.Save();
            return id;
        }

        // fetches the calibration result computed by a CameraCalibrationSink
        public CameraCalibrationResult GetCameraCalibrationResult(int calibratorSinkId)
        {
            return ManagerWrapper.Instance.GetCameraCalibrationResult(calibratorSinkId);
        }

        // saves the checkerboard corners detected in a CameraCalibrationSink's latest frame, to be
        // used later when computing the calibration result
        public bool SaveCameraCalibrationBoardDetection(int calibratorSinkId)
        {
            return ManagerWrapper.Instance.SaveCameraCalibrationBoardDetection(calibratorSinkId);
        }

        // explicitly runs cv::calibrateCamera over every snapshot saved so far - the UI decides
        // when this happens rather than it running implicitly on every result fetch
        public CameraCalibrationResult RunCameraCalibration(int calibratorSinkId)
        {
            var result = ManagerWrapper.Instance.RunCameraCalibration(calibratorSinkId);
            CalibrationManager.Instance.SaveResult(calibratorSinkId, result);
            return result;
        }

        public int GetCameraCalibrationSnapshotCount(int calibratorSinkId) =>
            ManagerWrapper.Instance.GetCameraCalibrationSnapshotCount(calibratorSinkId);

        public bool RemoveCameraCalibrationSnapshot(int calibratorSinkId, int index) =>
            ManagerWrapper.Instance.RemoveCameraCalibrationSnapshot(calibratorSinkId, index);

        public void ClearCameraCalibrationSnapshots(int calibratorSinkId) =>
            ManagerWrapper.Instance.ClearCameraCalibrationSnapshots(calibratorSinkId);

        // creates an ApriltagSink that reuses the calibration result of an existing CameraCalibrationSink,
        // so the apriltag detections can be translated into real world tag locations
        public int AddApriltagSinkFromCalibrator(string name, int calibratorSinkId, double tagSize)
        {
            int id = ManagerWrapper.Instance.CreateApriltagDetectorFromCalibrator(calibratorSinkId, tagSize);
            sinks.Add(new Sink(id, name, SinkType.ApriltagSink));
            DB.Instance.Save();
            return id;
        }

        // creates an ApriltagSink with an explicit backend selection (CPU or Vulkan) and no
        // calibration data - use AddApriltagSinkFromCalibrator, or bind+calibrate afterwards,
        // to get real-world pose. frameWidth/frameHeight only matter for the Vulkan backend.
        public int AddApriltagSinkWithBackend(string name, double tagSize, ApriltagBackendKind backend, int frameWidth, int frameHeight)
        {
            int id = ManagerWrapper.Instance.CreateApriltagDetector(new CameraCalibrationResult(), tagSize, backend, frameWidth, frameHeight);
            sinks.Add(new Sink(id, name, SinkType.ApriltagSink));
            DB.Instance.Save();
            return id;
        }

        // reports which backend an ApriltagSink actually ended up running - may differ from
        // what was requested if Vulkan was asked for and no usable device was found
        public string GetApriltagBackendName(int sinkId)
        {
            return ManagerWrapper.Instance.GetApriltagDetectorBackendName(sinkId);
        }

        // creates a WebRTCSink; bind it (BindSourceToSink) to any single frame-producing node -
        // a raw camera, or a detector's annotated output - to stream that stage. encoderName
        // defaults to null (resolved to GetPreferredWebRTCEncoder() below), not a hardcoded
        // "libx264" - see WebRTCSinkController.Create's own comment for why.
        public int AddWebRTCSink(string name, int bitrateKbps = 4000, int fps = 30, string? encoderName = null)
        {
            int id = ManagerWrapper.Instance.CreateWebRTCSink(bitrateKbps, fps, encoderName ?? ManagerWrapper.Instance.GetPreferredWebRTCEncoder());
            sinks.Add(new Sink(id, name, SinkType.WebRTCSink));
            DB.Instance.Save();
            return id;
        }

        public string WebRTCCreateOffer(int sinkId) => ManagerWrapper.Instance.WebRTCCreateOffer(sinkId);
        public void WebRTCSetAnswer(int sinkId, string sdp) => ManagerWrapper.Instance.WebRTCSetAnswer(sinkId, sdp);
        public void WebRTCAddIceCandidate(int sinkId, string candidate, string mid) =>
            ManagerWrapper.Instance.WebRTCAddIceCandidate(sinkId, candidate, mid);
        public bool IsWebRTCSinkConnected(int sinkId) => ManagerWrapper.Instance.IsWebRTCSinkConnected(sinkId);
        public string GetWebRTCSinkStatus(int sinkId) => ManagerWrapper.Instance.GetWebRTCSinkStatus(sinkId);

        // creates an ObjectDetectionSink running a previously uploaded model. Provider is fixed
        // to ONNX for now - RKNN is accepted by the native API but throws (not implemented yet).
        public int AddObjectDetectionSink(string name, int modelId)
        {
            var model = ModelManager.Instance.GetModel(modelId);
            if (model == null) throw new ArgumentException($"no model with id {modelId}");

            int id = ManagerWrapper.Instance.CreateObjectDetectionSink(
                ObjectDetectionProvider.ONNX, model.ModelPath, model.LabelsPath, model.Variant,
                model.ConfThreshold, model.NmsThreshold, model.InputSize);
            sinks.Add(new Sink(id, name, SinkType.ObjectDetectionSink));
            DB.Instance.Save();
            return id;
        }

        // ROADMAP.md Phase 7 (pipeline profiles): (re)creates the one detection sink a
        // PipelineProfile describes, applying every setting that has no live mutator on the
        // native side (tag size, calibration, backend selection, model choice) at construction
        // time and everything else (field layout, driver mode) via its own setter immediately
        // after. When explicitId is set, creates at that exact id via the Manager overloads that
        // accept one explicitly - used by SourceManager.ActivateProfile to preserve a source's
        // ActiveDetectionSinkId across a profile switch, so nothing downstream needs rebinding
        // by id (see that method's own comment on why bindings still need re-establishing even
        // so - deleting the old sink at that id unbinds them natively regardless).
        public int CreateOrReplaceDetectionSinkForProfile(string name, PipelineProfile profile, int? explicitId)
        {
            int id;
            switch (profile.Kind)
            {
                case DetectionSinkKind.ApriltagSink:
                    CameraCalibrationResult calibration = profile.CalibratorSinkId.HasValue
                        ? ManagerWrapper.Instance.GetCameraCalibrationResult(profile.CalibratorSinkId.Value)
                        : new CameraCalibrationResult();
                    double tagSize = profile.TagSize ?? 0.1651;
                    ApriltagBackendKind backend = profile.Backend ?? ApriltagBackendKind.APRILTAG_BACKEND_CPU;
                    id = explicitId.HasValue
                        ? ManagerWrapper.Instance.CreateApriltagDetector(explicitId.Value, calibration, tagSize,
                            backend, profile.FrameWidth, profile.FrameHeight)
                        : ManagerWrapper.Instance.CreateApriltagDetector(calibration, tagSize,
                            backend, profile.FrameWidth, profile.FrameHeight);
                    sinks.Add(new Sink(id, name, SinkType.ApriltagSink));

                    if (!string.IsNullOrEmpty(profile.FieldLayoutPath))
                        ManagerWrapper.Instance.LoadFieldLayout(id, profile.FieldLayoutPath);
                    ManagerWrapper.Instance.SetDriverMode(id, profile.DriverMode);
                    break;

                case DetectionSinkKind.ObjectDetectionSink:
                    if (!profile.ModelId.HasValue)
                        throw new ArgumentException("ObjectDetectionSink profile has no ModelId set");
                    var model = ModelManager.Instance.GetModel(profile.ModelId.Value);
                    if (model == null) throw new ArgumentException($"no model with id {profile.ModelId.Value}");

                    id = explicitId.HasValue
                        ? ManagerWrapper.Instance.CreateObjectDetectionSink(explicitId.Value, ObjectDetectionProvider.ONNX,
                            model.ModelPath, model.LabelsPath, model.Variant, model.ConfThreshold, model.NmsThreshold, model.InputSize)
                        : ManagerWrapper.Instance.CreateObjectDetectionSink(ObjectDetectionProvider.ONNX,
                            model.ModelPath, model.LabelsPath, model.Variant, model.ConfThreshold, model.NmsThreshold, model.InputSize);
                    sinks.Add(new Sink(id, name, SinkType.ObjectDetectionSink));
                    break;

                default:
                    throw new ArgumentException($"unknown pipeline profile kind {profile.Kind}");
            }

            DB.Instance.Save();
            return id;
        }

        // every sink currently bound (as its source) to sourceId - used by
        // SourceManager.ActivateProfile to find the downstream sinks (WebRTC preview,
        // NetworkTablesSink, etc.) that were reading a detection sink's output before it gets
        // torn down and recreated, so they can be rebound afterwards.
        public List<int> GetSinksBoundToSource(int sourceId)
        {
            return sinks.Where(s => s.Source != null && s.Source.Id == sourceId).Select(s => s.Id).ToList();
        }

        // creates a NetworkTablesSink that connects to a server via team number (e.g. 1234 ->
        // roboRIO mDNS/static IP resolution, exactly like a real driver station)
        public int AddNetworkTablesSinkForTeam(string name, int teamNumber, string rootTable, string clientIdentity)
        {
            int id = ManagerWrapper.Instance.CreateNetworkTablesSinkForTeam(teamNumber, rootTable, clientIdentity);
            sinks.Add(new Sink(id, name, SinkType.NetworkTablesSink));
            DB.Instance.Save();
            return id;
        }

        // creates a NetworkTablesSink that connects to an explicit server address - useful for
        // bench testing against a local NT4 server/Glass instance instead of a real robot
        public int AddNetworkTablesSinkForServer(string name, string serverAddress, int port, string rootTable, string clientIdentity)
        {
            int id = ManagerWrapper.Instance.CreateNetworkTablesSinkForServer(serverAddress, port, rootTable, clientIdentity);
            sinks.Add(new Sink(id, name, SinkType.NetworkTablesSink));
            DB.Instance.Save();
            return id;
        }

        public bool IsNetworkTablesSinkConnected(int sinkId)
        {
            return ManagerWrapper.Instance.IsNetworkTablesSinkConnected(sinkId);
        }

        public string GetNetworkTablesSinkStatus(int sinkId)
        {
            return ManagerWrapper.Instance.GetNetworkTablesSinkStatus(sinkId);
        }

        // --- Stereo depth (phase 10) - see STEREO_IMPLEMENTATION_PLAN.md ---

        public int AddStereoCalibrationSink(string name)
        {
            int id = ManagerWrapper.Instance.CreateStereoCalibrator();
            sinks.Add(new Sink(id, name, SinkType.StereoCalibrationSink));
            DB.Instance.Save();
            return id;
        }

        // explicit board config - default is a 6x9 checkerboard, 25mm squares, matching
        // CreateStereoCalibrator()'s own native default (ChArUco isn't supported for stereo -
        // see StereoCalibrator.h - so unlike CameraCalibrator there's no marker size/dictionary
        // parameter here)
        public int AddStereoCalibrationSinkWithBoard(string name, CalibrationBoardType boardType, int rows, int cols, float squareSizeMeters)
        {
            int id = ManagerWrapper.Instance.CreateStereoCalibrator(boardType, rows, cols, squareSizeMeters);
            sinks.Add(new Sink(id, name, SinkType.StereoCalibrationSink));
            DB.Instance.Save();
            return id;
        }

        // binds the explicit left/right roles of a stereo sink (StereoCalibrationSink or
        // StereoDepthSink) - ordinary BindSourceToSink is bind-order only and has no left/right
        // notion at all, so getting the two backwards would silently flip the sign of every
        // disparity (see BindStereoSources' own comment in Manager.h/IStereoRoleReceiver.h).
        public void BindStereoSourcesToSink(int sinkId, int leftSourceId, int rightSourceId)
        {
            var sink = sinks.FirstOrDefault(s => s.Id == sinkId);
            if (sink == null) throw new ArgumentException($"no sink with id {sinkId}");

            Source ResolveSource(int sourceId)
            {
                Source? s = SourceManager.Instance.GetSourceById(sourceId);
                if (s == null) {
                    // dual-role sink acting as its own source - see BindSourceToSink's own
                    // DualRoleSinkTypes comment
                    var sourceSink = sinks.FirstOrDefault(sk => sk.Id == sourceId && DualRoleSinkTypes.Contains(sk.Type));
                    if (sourceSink != null) s = new Source(sourceSink.Id, sourceSink.Name, SourceType.SinkOutput);
                }
                if (s == null) throw new Exception($"no source (or dual-role sink) with id {sourceId}");
                return s;
            }

            Source left = ResolveSource(leftSourceId);
            Source right = ResolveSource(rightSourceId);

            bool ok = ManagerWrapper.Instance.BindStereoSources(sinkId, leftSourceId, rightSourceId);
            if (!ok) throw new Exception($"BindStereoSources failed for sink {sinkId}");

            sink.Source = left;
            sink.Source2 = right;

            // same reasoning as BindSourceToSink: only a "real" SourceManager-tracked source has
            // its own enable/disable lifecycle to kick off here
            if (SourceManager.Instance.GetSourceById(leftSourceId) != null) SourceManager.Instance.EnableSourceById(leftSourceId);
            if (SourceManager.Instance.GetSourceById(rightSourceId) != null) SourceManager.Instance.EnableSourceById(rightSourceId);

            DB.Instance.Save();
        }

        public bool SaveStereoCalibrationDetection(int calibratorSinkId) =>
            ManagerWrapper.Instance.SaveStereoCalibrationDetection(calibratorSinkId);

        public int GetStereoCalibrationPairCount(int calibratorSinkId) =>
            ManagerWrapper.Instance.GetStereoCalibrationPairCount(calibratorSinkId);

        public bool RemoveStereoCalibrationPair(int calibratorSinkId, int index) =>
            ManagerWrapper.Instance.RemoveStereoCalibrationPair(calibratorSinkId, index);

        public void ClearStereoCalibrationPairs(int calibratorSinkId) =>
            ManagerWrapper.Instance.ClearStereoCalibrationPairs(calibratorSinkId);

        // explicitly runs cv::stereoCalibrate + cv::stereoRectify over every pair saved so far,
        // and persists the result (keyed by both cameras' device paths + resolution) if the
        // sink is bound to two real camera sources.
        public StereoCalibrationResult RunStereoCalibration(int calibratorSinkId)
        {
            var result = ManagerWrapper.Instance.RunStereoCalibration(calibratorSinkId);
            StereoCalibrationManager.Instance.SaveResult(calibratorSinkId, result);
            return result;
        }

        public StereoCalibrationResult GetStereoCalibrationResult(int calibratorSinkId) =>
            ManagerWrapper.Instance.GetStereoCalibrationResult(calibratorSinkId);

        // creates a StereoDepthNode bound to nothing yet - bind its left/right sources with
        // BindStereoSourcesToSink afterwards. `calibration` is normally the result of
        // RunStereoCalibration/GetStereoCalibrationResult on a StereoCalibrationSink.
        public int AddStereoDepthSink(string name, StereoDepthBackendKind backend, StereoCalibrationResult calibration,
            double minDepthMeters, double maxDepthMeters, int maxSkewUs, StereoFrameOutput frameOutput)
        {
            int id = ManagerWrapper.Instance.CreateStereoDepthNode(backend, calibration, minDepthMeters, maxDepthMeters, maxSkewUs, frameOutput);
            sinks.Add(new Sink(id, name, SinkType.StereoDepthSink));
            DB.Instance.Save();
            return id;
        }

        public string GetStereoDepthBackendName(int sinkId) => ManagerWrapper.Instance.GetStereoDepthBackendName(sinkId);
        public double GetStereoDepthValidFraction(int sinkId) => ManagerWrapper.Instance.GetStereoDepthValidFraction(sinkId);
        public double GetStereoDepthMedianDepthMeters(int sinkId) => ManagerWrapper.Instance.GetStereoDepthMedianDepthMeters(sinkId);

        // creates a DepthFusionNode - bind the detector (ObjectDetectionSink/ApriltagSink) with
        // the ordinary BindSourceToSink (it must itself be bound to the StereoDepthSink's own
        // rectified-left frame output, not a raw camera - see DepthFusionNode.h), then attach
        // the depth source separately via AttachDepthFusionSource.
        public int AddDepthFusionSink(string name)
        {
            int id = ManagerWrapper.Instance.CreateDepthFusionNode();
            sinks.Add(new Sink(id, name, SinkType.DepthFusionSink));
            DB.Instance.Save();
            return id;
        }

        // attaches the StereoDepthSink a DepthFusionNode reads its depth grid from directly -
        // not a normal bind (see DepthFusionNode.h: the full depth grid is never serialized
        // through SourceResult/JSON, so this is a distinct, direct C++ reference).
        public void AttachDepthFusionSource(int fusionSinkId, int stereoDepthSinkId)
        {
            bool ok = ManagerWrapper.Instance.SetDepthFusionDepthNode(fusionSinkId, stereoDepthSinkId);
            if (!ok) throw new Exception($"AttachDepthFusionSource failed for fusion sink {fusionSinkId} / depth sink {stereoDepthSinkId}");

            var sink = sinks.FirstOrDefault(s => s.Id == fusionSinkId);
            if (sink != null)
            {
                sink.DepthSourceId = stereoDepthSinkId;
                DB.Instance.Save();
            }
        }

        // update results
        private void updateResults()
        {
            string[] results = getAllSinkIds().Select(id => ManagerWrapper.Instance.GetSinkResult(id)).ToArray();
            
            // Manually construct JSON array without re-serializing the JSON strings
            if (results.Length == 0)
            {
                currentResults = "[]";
            }
            else
            {
                currentResults = "[" + string.Join(",", results) + "]";
            }
        }

        // start the sink manager thread
        public void EnableManagerThread()
        {
            if (!isRunning)
            {
                isRunning = true;
                thread = new Thread(ThreadProc); // Recreate the thread if it has been stopped
                thread.Start();
            }
        }

        // stop the sink manager thread
        public void DisableManagerThread()
        {
            if (thread.IsAlive)
            {
                isRunning = false;
                thread.Join();
            }
        }

        // stop sink by id
        public void DisableSinkById(int id)
        {
            Source source = null;
            foreach (var sink in sinks)
            {
                if (sink.Id == id)
                {
                    if (sink.Source != null)
                    {
                        source = sink.Source;
                    }

                    ManagerWrapper.Instance.StopSinkById(id);
                    break;
                }
            }

            if (source != null)
            {
                bool isSourceUsed = false;
                foreach (var s in sinks)
                {
                    if (s.Source != null && s.Source.Id == source.Id)
                    {
                        isSourceUsed = true;
                        break;
                    }
                }

                if (!isSourceUsed)
                {
                    SourceManager.Instance.DisableSourceById(source.Id);
                }
            }
            // Logic to stop a sink by its ID
            // This could involve finding the sink in the sinks list and stopping it.
        }

        // start sink by id
        public void EnableSinkById(int id)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == id)
                {
                    if (sink.Source != null)
                    {
                        SourceManager.Instance.EnableSourceById(sink.Source.Id);
                    }

                    ManagerWrapper.Instance.StartSinkById(id);
                    return;
                }
            }
        }

        public void EnableAllSinks()
        {
            foreach (var sink in sinks)
            {
                if (sink.Source != null)
                {
                    SourceManager.Instance.EnableSourceById(sink.Source.Id);
                }

                ManagerWrapper.Instance.StartSinkById(sink.Id);
            }
        }

        public void UnbindSourceFromSink(int sinkId, int? sourceId = null)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == sinkId)
                {
                    ManagerWrapper.Instance.UnbindSourceFromSink(sinkId);
                    sink.Source = null; // Unbind the source
                    DB.Instance.Save(); // Save changes to the database
                    break;
                }
            }
        }

        // ApriltagSink, ObjectDetectionSink and CameraCalibrationSink are dual-role: natively
        // registered as both a sink AND a source (see the m_Sources.emplace calls alongside
        // m_Sinks.emplace in Manager.cpp's CreateApriltagDetector/CreateObjectDetectionSink/
        // CreateCameraCalibrator), so their own id is a perfectly valid bind target for e.g. a
        // WebRTCSink wanting to preview a detector's annotated output. SourceManager's C# source
        // list never tracked these though - only real camera/video/image sources - so binding to
        // one used to look up a null Source here and NullReferenceException on the line below.
        private static readonly HashSet<SinkType> DualRoleSinkTypes = new HashSet<SinkType> {
            SinkType.ApriltagSink, SinkType.ObjectDetectionSink, SinkType.CameraCalibrationSink,
            SinkType.StereoCalibrationSink, SinkType.StereoDepthSink, SinkType.DepthFusionSink
        };

        public void BindSourceToSink(int sinkId, int sourceId)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == sinkId)
                {
                    Source? source = SourceManager.Instance.GetSourceById(sourceId);
                    bool isRealSource = source != null;
                    if (source == null)
                    {
                        var sourceSink = sinks.FirstOrDefault(s => s.Id == sourceId && DualRoleSinkTypes.Contains(s.Type));
                        if (sourceSink != null) source = new Source(sourceSink.Id, sourceSink.Name, SourceType.SinkOutput);
                    }
                    if (source == null) throw new Exception($"no source (or dual-role sink) with id {sourceId}");

                    sink.Source = source;
                    ManagerWrapper.Instance.BindSourceToSink(sourceId, sinkId);
                    // Only a "real" SourceManager-tracked source has its own enable/disable
                    // lifecycle to kick off here - a dual-role sink's underlying node is already
                    // started/stopped via its own Enabled toggle (EnableSinkById), not this one.
                    if (isRealSource) SourceManager.Instance.EnableSourceById(source.Id);
                    DB.Instance.Save(); // Save changes to the database
                    break;
                }
            }
        }

        public bool IsSinkRunning(int id)
        {
            return ManagerWrapper.Instance.IsSinkActive(id);
        }
    }
}
