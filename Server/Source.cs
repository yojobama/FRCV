using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public enum SourceType
    {
        Camera,
        ImageFile,
        VideoFile,
        // A dual-role sink (ApriltagSink, ObjectDetectionSink, CameraCalibrationSink) acting as
        // the frame/json-producing side of itself - natively these are already registered in
        // Manager's own m_Sources map (see Manager.cpp), but SourceManager's C# source list only
        // ever tracked "real" sources (camera/video/image), so a WebRTCSink or NetworkTablesSink
        // could never be bound to e.g. an AprilTag detector's own output - confirmed the hard way,
        // this is why WebRTC preview against the AprilTag detector didn't work. See
        // SinkManager.BindSourceToSink.
        SinkOutput
    }

    public class Source
    {
        private int id;
        private string name;
        private SourceType type;
        
        public CameraHardwareInfo? CameraHardwareInfo { get; set; } // Optional camera hardware info for camera sources
        public int? Fps { get; set; } // Optional frames per second for camera sources
        public string? FilePath { get; set; } // Optional file path for image or video sources

        // ROADMAP.md Phase 7: pipeline profiles - see PipelineProfile.cs for the full design.
        // ActiveDetectionSinkId is the stable sink id every profile activation reuses (created
        // fresh the first time a profile is ever activated for this source, then torn down and
        // recreated in place on every subsequent switch) so downstream bindings never have to
        // change. ActiveProfileIndex is -1 when no profile has been activated yet.
        public List<PipelineProfile> Profiles { get; set; } = new List<PipelineProfile>();
        public int ActiveProfileIndex { get; set; } = -1;
        public int? ActiveDetectionSinkId { get; set; }

        public SourceType Type
        {
            get { return type; }
        }

        public int Id
        {
            get => id; set => id = value;
        }
        
        public string Name
        {
            get => name;
            set => name = value;
        }

        public Source(int id, string name, SourceType type, string? filePath = null, int? fps = null, CameraHardwareInfo? cameraHardwareInfo = null)
        {
            this.id = id;
            this.name = name;
            this.type = type;
            this.FilePath = filePath;
            this.Fps = fps;
            this.CameraHardwareInfo = cameraHardwareInfo;
        }
    }
}
