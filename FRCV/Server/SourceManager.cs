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
    }
}
