using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public enum SinkType
    {
        ApriltagSink,
        ObjectDetectionSink,
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
                        id = ManagerWrapper.Instance.createApriltagSink();
                        break;
                    case SinkType.ObjectDetectionSink:
                        id = ManagerWrapper.Instance.createObjectDetectionSink();
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
                    id = ManagerWrapper.Instance.createApriltagSink();
                    break;
                case SinkType.ObjectDetectionSink:
                    id = ManagerWrapper.Instance.createObjectDetectionSink();
                    break;
            }
        }
    }
}
