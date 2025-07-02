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

    internal class Sink
    {
        SinkType type { get; set; }
        int id { get; set; }
        string name { get; set; }

        public Sink(int id, string name, SinkType type)
        {
            this.id = id;
            this.name = name;
            this.type = type;
        }
    }
}
