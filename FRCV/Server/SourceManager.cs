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
                ManagerWrapper.Instance.stopSourceById(source.Id); // Assuming Source has an isEnabled property
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
                ManagerWrapper.Instance.startSourceById(source.Id); // Assuming Source has an isEnabled property
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
                    ManagerWrapper.Instance.stopSourceById(source.Id); // Assuming Source has an isEnabled property
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
                    ManagerWrapper.Instance.startSourceById(source.Id); // Assuming Source has an isEnabled property
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

        public void InitializeCameraSource(CameraHardwareInfo cameraHardwareInfo, string name = "default")
        {
            sources.Add(new Source(ManagerWrapper.Instance.createCameraSource(cameraHardwareInfo), name, SourceType.Camera));
        }

        public void InitializeVideoFileSource(string filePath, int fps = 30, string name = "default")
        {
            sources.Add(new Source(ManagerWrapper.Instance.createVideoFileSource(filePath, fps), name, SourceType.VideoFile));
        }
        
        public void initializeImageFileSource(string filePath, string name = "default")
        {
            sources.Add(new Source(ManagerWrapper.Instance.createImageFileSource(filePath), name, SourceType.ImageFile));
        }
    }
}
