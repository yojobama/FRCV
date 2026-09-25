using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;

namespace Server
{
    public class SourceManager
    {
        public static SourceManager Instance { get; } = new SourceManager();

        private List<Source> sources;

        private SourceManager()
        {
            sources = new List<Source>();
            // Initialize the source manager
            // This could include setting up data sources, initializing channels, etc.
        }

        public void ChangeSourceName(int sourceId, string newName)
        {
            Source source = GetSourceById(sourceId);
            if (source != null)
            {
                source.Name = newName;
                DB.Instance.Save(); // Save changes to the database
            }
        }

        public int[] GetAllSourceIds()
        {
            List<int> ids = new List<int>();
            foreach (var source in sources)
            {
                ids.Add(source.Id);
            }
            return ids.ToArray();
        }

        public void DisableAllSources()
        {
            foreach (var source in sources)
            {
                // Logic to disable the source
                // This could involve setting its state to stopped and releasing any resources it holds.
                ManagerWrapper.Instance.StopSourceById(source.Id); // Assuming Source has an isEnabled property
            }
            // Logic to stop all sources
            // This could involve iterating through a list of sources and stopping each one.
        }
        public void EnableAllSources()
        {
            foreach (var source in sources)
            {
                // Logic to enable the source
                // This could involve setting its state to running and initializing any necessary resources.
                ManagerWrapper.Instance.StartSourceById(source.Id); // Assuming Source has an isEnabled property
            }
            // Logic to start all sources
            // This could involve iterating through a list of sources and starting each one.
        }

        public void DisableSourceById(int id)
        {
            foreach (var source in sources)
            {
                if (source.Id == id)
                {
                    // Logic to disable the source
                    // This could involve setting its state to stopped and releasing any resources it holds.
                    ManagerWrapper.Instance.StopSourceById(source.Id); // Assuming Source has an isEnabled property
                    return;
                }
            }
            // Logic to disable a source by its ID
            // This could involve finding the source in a list and disabling it.
        }
        public void EnableSourceById(int id)
        {
            foreach (var source in sources)
            {
                if (source.Id == id)
                {
                    // Logic to enable the source
                    // This could involve setting its state to running and initializing any necessary resources.
                    if (!ManagerWrapper.Instance.StartSourceById(source.Id)) throw new Exception("unable to start source"); // Assuming Source has an isEnabled property
                    return;
                }
            }
            // Logic to enable a source by its ID
            // This could involve finding the source in a list and enabling it.
        }

        public Source GetSourceById(int id)
        {
            foreach (var source in sources)
            {
                if (source.Id == id)
                {
                    return source;
                }
            }
            return null;
        }

        public int InitializeCameraSource(CameraHardwareInfo cameraHardwareInfo, string name = "default", int? id = null)
        {
            int sourceId = -1;

            if (id.HasValue)
                sourceId = ManagerWrapper.Instance.CreateCameraSource(cameraHardwareInfo, id.Value);
            else
                sourceId = ManagerWrapper.Instance.CreateCameraSource(cameraHardwareInfo);

            sources.Add(new Source(sourceId, name, SourceType.Camera, cameraHardwareInfo: cameraHardwareInfo));
            DB.Instance.Save(); // Save changes to the database
            return sourceId;
        }

        public int InitializeVideoFileSource(string filePath, int fps = 30, string name = "default", int? id = null)
        {
            int sourceId = -1;

            if (id.HasValue)
                sourceId = ManagerWrapper.Instance.CreateVideoFileSource(filePath, fps, id.Value);
            else
                sourceId = ManagerWrapper.Instance.CreateVideoFileSource(filePath, fps);
            
            sources.Add(new Source(sourceId, name, SourceType.VideoFile, filePath, fps));
            DB.Instance.Save(); // Save changes to the database
            return sourceId;
        }
        
        public int initializeImageFileSource(string filePath, string name = "default", int? id = null)
        {
            int sourceId = -1;
            
            if (id.HasValue)
                sourceId = ManagerWrapper.Instance.CreateImageFileSource(filePath, id.Value);
            else
                sourceId = ManagerWrapper.Instance.CreateImageFileSource(filePath);
            
            sources.Add(new Source(sourceId, name, SourceType.ImageFile, filePath));
            DB.Instance.Save(); // Save changes to the database
            return sourceId;
        }

        // NEW: delete a source and unbind it from any sinks referencing it
        public void DeleteSource(int sourceId)
        {
            ManagerWrapper.Instance.DeleteSource(sourceId);

            // Remove source from list
            sources.RemoveAll(s => s.Id == sourceId);

            // Unbind from any sinks that referenced it
            foreach (var sinkId in SinkManager.Instance.getAllSinkIds())
            {
                var sink = SinkManager.Instance.GetSinkById(sinkId);
                if (sink != null && sink.Source != null && sink.Source.Id == sourceId)
                {
                    sink.Source = null;
                }
            }

            DB.Instance.Save();
        }

        public bool IsSourceActive(int sourceId)
        {
            return ManagerWrapper.Instance.IsSourceActive(sourceId);
        }

        // ROADMAP.md Phase 7: pipeline profiles - see PipelineProfile.cs for the design this
        // implements. Index is assigned once and never reused after a delete, matching
        // PhotonVision's own pipelineIndex semantics (a robot program's stored index must keep
        // meaning the same profile even after an unrelated one is removed).
        public int AddApriltagProfile(int sourceId, string name, double tagSize, int? calibratorSinkId,
            ApriltagBackendKind backend, int frameWidth, int frameHeight, bool driverMode,
            int? threads = null, float? quadDecimate = null, bool? refineEdges = null)
        {
            Source source = GetSourceById(sourceId) ?? throw new ArgumentException($"no source with id {sourceId}");
            int index = source.Profiles.Count == 0 ? 0 : source.Profiles.Max(p => p.Index) + 1;
            source.Profiles.Add(new PipelineProfile
            {
                Index = index,
                Name = name,
                Kind = DetectionSinkKind.ApriltagSink,
                TagSize = tagSize,
                CalibratorSinkId = calibratorSinkId,
                Backend = backend,
                FrameWidth = frameWidth,
                FrameHeight = frameHeight,
                DriverMode = driverMode,
                Threads = threads,
                QuadDecimate = quadDecimate,
                RefineEdges = refineEdges
            });
            DB.Instance.Save();
            return index;
        }

        public int AddObjectDetectionProfile(int sourceId, string name, int modelId)
        {
            Source source = GetSourceById(sourceId) ?? throw new ArgumentException($"no source with id {sourceId}");
            int index = source.Profiles.Count == 0 ? 0 : source.Profiles.Max(p => p.Index) + 1;
            source.Profiles.Add(new PipelineProfile
            {
                Index = index,
                Name = name,
                Kind = DetectionSinkKind.ObjectDetectionSink,
                ModelId = modelId
            });
            DB.Instance.Save();
            return index;
        }

        public void DeleteProfile(int sourceId, int index)
        {
            Source source = GetSourceById(sourceId) ?? throw new ArgumentException($"no source with id {sourceId}");
            if (source.ActiveProfileIndex == index)
                throw new InvalidOperationException("cannot delete the active profile - activate a different one first");
            source.Profiles.RemoveAll(p => p.Index == index);
            DB.Instance.Save();
        }

        public List<PipelineProfile> GetProfiles(int sourceId)
        {
            Source source = GetSourceById(sourceId) ?? throw new ArgumentException($"no source with id {sourceId}");
            return source.Profiles;
        }

        // writes a field layout JSON onto one profile (not the sink it may currently be running
        // as - see PipelineProfile.FieldLayoutPath's own comment on why this is profile-scoped),
        // and if that profile happens to be the active one, applies it to the live sink
        // immediately so a caller doesn't have to reactivate the same index just to pick it up.
        public void SetProfileFieldLayout(int sourceId, int index, string path)
        {
            Source source = GetSourceById(sourceId) ?? throw new ArgumentException($"no source with id {sourceId}");
            PipelineProfile profile = source.Profiles.FirstOrDefault(p => p.Index == index)
                ?? throw new ArgumentException($"source {sourceId} has no profile at index {index}");
            profile.FieldLayoutPath = path;
            DB.Instance.Save();

            if (source.ActiveProfileIndex == index && source.ActiveDetectionSinkId.HasValue)
                ManagerWrapper.Instance.LoadFieldLayout(source.ActiveDetectionSinkId.Value, path);
        }

        // Tears down whatever detection sink is currently running for this source (if any) and
        // recreates it from the chosen profile's settings, AT THE SAME sink id
        // (source.ActiveDetectionSinkId) once one has ever been assigned. Preserving that id is
        // what lets a WebRTC preview or NetworkTablesSink stay configured against "this source's
        // detection output" across a switch rather than needing to be re-pointed every time a
        // profile changes - but Manager::DeleteSink natively unbinds every other sink from the
        // one it deletes (it walks m_Sinks and calls UnbindSource on each), so those downstream
        // bindings are captured before the delete and explicitly re-established after the
        // replacement sink comes up at the same id.
        public void ActivateProfile(int sourceId, int profileIndex)
        {
            Source source = GetSourceById(sourceId) ?? throw new ArgumentException($"no source with id {sourceId}");
            PipelineProfile profile = source.Profiles.FirstOrDefault(p => p.Index == profileIndex)
                ?? throw new ArgumentException($"source {sourceId} has no profile at index {profileIndex}");

            List<int> downstreamSinkIds = source.ActiveDetectionSinkId.HasValue
                ? SinkManager.Instance.GetSinksBoundToSource(source.ActiveDetectionSinkId.Value)
                : new List<int>();

            int? explicitId = source.ActiveDetectionSinkId;
            if (explicitId.HasValue)
                SinkManager.Instance.DeleteSink(explicitId.Value);

            string sinkName = $"{source.Name} - {profile.Name}";
            int sinkId = SinkManager.Instance.CreateOrReplaceDetectionSinkForProfile(sinkName, profile, explicitId);

            SinkManager.Instance.BindSourceToSink(sinkId, sourceId);
            foreach (int downstreamId in downstreamSinkIds)
            {
                SinkManager.Instance.BindSourceToSink(downstreamId, sinkId);
            }
            SinkManager.Instance.EnableSinkById(sinkId);

            source.ActiveDetectionSinkId = sinkId;
            source.ActiveProfileIndex = profileIndex;
            DB.Instance.Save();
        }

        public int GetActiveProfileIndex(int sourceId)
        {
            Source source = GetSourceById(sourceId) ?? throw new ArgumentException($"no source with id {sourceId}");
            return source.ActiveProfileIndex;
        }
    }
}
