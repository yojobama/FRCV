using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public enum SinkType
    {
        [Description("ApriltagSink")]
        ApriltagSink,
        [Description("ObjectDetectionSink")]
        ObjectDetectionSink,
        [Description("RecordingSink")]
        RecordingSink,
        [Description("CameraCalibrationSink")]
        CameraCalibrationSink,
    }

    public class Sink
    {
       // class members 
        private SinkType type { get; set; }
        private int id { get; set; }
        private string name { get; set; }
        
        private Source? source;
        
        // properties for the sink

        public SinkType Type
        {
            get => type;
            set
            {
                switch (value)
                {
                    case SinkType.ApriltagSink:
                        id = ManagerWrapper.Instance.CreateApriltagSink();
                        break;
                    case SinkType.ObjectDetectionSink:
                        id = ManagerWrapper.Instance.CreateObjectDetectionSink();
                        break;
                }
                type = value;
            }
        }

        public int Id
        {
            get => id;
        }

        public string Name
        {
            get => name;
            set => name = value;
        }
        
        public Source? Source
        {
            get => source;
            set => source = value;
        }

        public Sink(int id, string name, SinkType type, Source? source = null)
        {
            this.id = id;
            this.name = name;
            this.type = type;
        }

        public void ChangeType(SinkType type)
        {
            switch (type)
            {
                case SinkType.ApriltagSink:
                    id = ManagerWrapper.Instance.CreateApriltagSink();
                    break;
                case SinkType.ObjectDetectionSink:
                    id = ManagerWrapper.Instance.CreateObjectDetectionSink();
                    break;
            }
        }
    }
}
