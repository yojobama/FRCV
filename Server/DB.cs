using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace Server
{
    public class DB
    {
        private class DBData
        {
            public List<Sink> Sinks { get; set; }
            public List<Source> Sources { get; set; }
        }

        public static DB Instance { get; } = new DB("data.json");

        private string jsonPath;
        private List<Sink> sinks;
        private List<Source> sources;
        private readonly Logger logger;

        private DB(string jsonPath)
        {
            this.jsonPath = jsonPath;
            sinks = new List<Sink>();
            sources = new List<Source>();
            logger = new Logger("DBLog.txt"); // Initialize logger with a log file
        }

        public string GetJson()
        {
            var dbData = new DBData
            {
                Sinks = sinks,
                Sources = sources
            };
            return JsonSerializer.Serialize(dbData, new JsonSerializerOptions { WriteIndented = true });
        }

        public void Load()
        {
            logger.EnterLog("DB Load called");
            if (File.Exists(jsonPath))
            {
                string jsonData = File.ReadAllText(jsonPath);
                try {
                    var dbData = JsonSerializer.Deserialize<DBData>(jsonData);

                    if (dbData != null)
                    {
                        sinks = dbData.Sinks ?? new List<Sink>();
                        sources = dbData.Sources ?? new List<Source>();
                    }

                    foreach (var source in sources)
                    {
                        switch (source.Type)
                        {
                            case SourceType.ImageFile:
                                SourceManager.Instance.initializeImageFileSource(source.FilePath, source.Name, source.Id);
                                break;
                            case SourceType.VideoFile:
                                SourceManager.Instance.InitializeVideoFileSource(source.FilePath, source.Fps ?? 30, source.Name, source.Id);
                                break;
                            case SourceType.Camera:
                                SourceManager.Instance.InitializeCameraSource(source.CameraHardwareInfo, id: source.Id);
                                break;
                        }

                        // InitializeXxxSource above builds its OWN fresh Source object (native
                        // creation always needs to run regardless of what's in the JSON), so the
                        // pipeline-profile fields deserialized onto this loop's own `source`
                        // never reach SourceManager's copy unless copied across explicitly here.
                        Source restored = SourceManager.Instance.GetSourceById(source.Id);
                        if (restored != null)
                        {
                            restored.Profiles = source.Profiles ?? new List<PipelineProfile>();
                            restored.ActiveProfileIndex = source.ActiveProfileIndex;
                            restored.ActiveDetectionSinkId = source.ActiveDetectionSinkId;
                        }
                    }
                    foreach (var sink in sinks)
                    {
                        SinkManager.Instance.AddSink(sink.Name, sink.Type.ToString(), sink.Id);
                    }

                    // AddSink only recreates the native node; the source->sink binding itself
                    // was never restored here, so every pipeline came back unbound after a
                    // restart (the robot power-cycles - this mattered). Sources must exist
                    // (created above) before rebinding.
                    foreach (var sink in sinks)
                    {
                        if (sink.Source != null && sink.Source2 != null)
                        {
                            // stereo sink (StereoCalibrationSink/StereoDepthSink) - Source is the
                            // left role, Source2 the right one; see Sink.Source2's own comment
                            SinkManager.Instance.BindStereoSourcesToSink(sink.Id, sink.Source.Id, sink.Source2.Id);
                        }
                        else if (sink.Source != null)
                        {
                            SinkManager.Instance.BindSourceToSink(sink.Id, sink.Source.Id);
                        }

                        if (sink.DepthSourceId.HasValue)
                        {
                            SinkManager.Instance.AttachDepthFusionSource(sink.Id, sink.DepthSourceId.Value);
                        }
                    }

                    // the robot power-cycles - a vision coprocessor that comes back up not
                    // actually running anything until an operator opens the WebUI defeats the
                    // point, for the sink types that are meant to run unattended. Deliberately
                    // NOT blanket: CameraCalibrationSink is an interactive, operator-driven
                    // wizard - auto-starting it just burns CPU hunting for a checkerboard with
                    // no one there to capture snapshots - and WebRTCSink's processing thread
                    // runs the H.264 encoder on every frame regardless of whether a peer is
                    // connected, so starting it before any client has even asked for a stream
                    // is pure waste. StartSinkById (re-)starts a sink's bound source too, which
                    // is safe now that ISource/ISink::Toggle are idempotent.
                    foreach (var sink in sinks)
                    {
                        // StereoCalibrationSink is interactive/operator-driven like
                        // CameraCalibrationSink - excluded for the same reason. StereoDepthSink
                        // is meant to run unattended (like ApriltagSink/ObjectDetectionSink), so
                        // it's not excluded here.
                        if (sink.Type != SinkType.CameraCalibrationSink && sink.Type != SinkType.WebRTCSink
                            && sink.Type != SinkType.StereoCalibrationSink)
                        {
                            SinkManager.Instance.EnableSinkById(sink.Id);
                        }
                    }

                    // ROADMAP.md Phase 7 (pipeline profiles): the loops above already recreated
                    // a source's ActiveDetectionSinkId generically (it's a perfectly ordinary
                    // entry in the persisted `sinks` list), but AddSink has no notion of a
                    // profile's own settings - tag size, calibration, field layout and driver
                    // mode would all silently come back at their defaults after a restart
                    // otherwise. ActivateProfile deletes and properly recreates it from the
                    // profile's real settings; the brief double-creation is harmless (once at
                    // startup) and reusing ActivateProfile here is what keeps this in sync with
                    // the exact same downstream-rebinding logic a live profile switch uses,
                    // rather than a second, easy-to-drift copy of it.
                    foreach (var source in sources)
                    {
                        if (source.ActiveProfileIndex >= 0 && source.Profiles.Any(p => p.Index == source.ActiveProfileIndex))
                        {
                            try
                            {
                                SourceManager.Instance.ActivateProfile(source.Id, source.ActiveProfileIndex);
                            }
                            catch (Exception ex)
                            {
                                logger.EnterLog($"Failed to reactivate pipeline profile {source.ActiveProfileIndex} for source {source.Id}: {ex.Message}");
                            }
                        }
                    }

                    logger.EnterLog("DB loaded successfully from " + jsonPath);
                }
                catch (Exception ex)
                {
                    logger.EnterLog("Error loading DB: " + ex.Message);
                }
            }
            else
            {
                logger.EnterLog("DB file not found: " + jsonPath);
            }
        }

        public void Save()
        {
            logger.EnterLog("DB Save called");
            List<Sink> sinks = new List<Sink>();
            List<Source> sources = new List<Source>();

            foreach (var sink in SinkManager.Instance.getAllSinkIds())
            {
                sinks.Add(SinkManager.Instance.GetSinkById(sink));
            }
            foreach (var source in SourceManager.Instance.GetAllSourceIds())
            {
                sources.Add(SourceManager.Instance.GetSourceById(source));
            }

            var dbData = new DBData
            {
                Sinks = sinks,
                Sources = sources
            };

            string jsonData = JsonSerializer.Serialize(dbData, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(jsonPath, jsonData);
            logger.EnterLog("DB saved successfully to " + jsonPath);
        }

        public void RemoveSink(Sink sink)
        {
            sinks.Remove(sink);
            Save();
        }

        public void RemoveSource(Source source)
        {
            sources.Remove(source);
            Save();
        }

        public List<Sink> GetSinks()
        {
            return sinks;
        }

        public List<Source> GetSources()
        {
            return sources;
        }

        public void Verify()
        {
            foreach (var sink in sinks)
            {
                if (SinkManager.Instance.GetSinkById(sink.Id) == null)
                {
                    SinkManager.Instance.AddSink(sink.Name, sink.Type.ToString());
                }
            }

            foreach (var source in sources)
            {
                if (SourceManager.Instance.GetSourceById(source.Id) == null)
                {
                    SourceManager.Instance.InitializeCameraSource(new CameraHardwareInfo { name = source.Name, path = "" });
                }
            }
        }
    }
}
