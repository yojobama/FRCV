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
                    case "ApriltagSink":
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
                }
                return id.GetValueOrDefault(-1);
            }
            else
            {
                switch (type)
                {
                    case "apriltag":
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
        // a raw camera, or a detector's annotated output - to stream that stage
        public int AddWebRTCSink(string name, int bitrateKbps = 4000, int fps = 30, string encoderName = "libx264")
        {
            int id = ManagerWrapper.Instance.CreateWebRTCSink(bitrateKbps, fps, encoderName);
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
            SinkType.ApriltagSink, SinkType.ObjectDetectionSink, SinkType.CameraCalibrationSink
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
