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
        public SinkType type { get; set; }
        public int id { get; set; }
        public Source source { get; set; }
        public string name { get; set; }

        public Sink(int id, string name, SinkType type, Source? source = null)
        {
            this.id = id;
            this.name = name;
            this.type = type;
            this.source = source;
        }
    }
}
