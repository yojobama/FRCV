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
        VideoFile
    }

    internal class Source
    {
        public int id { get; set; }
        public string name { get; set; }
        public SourceType type { get; set; }

        public Source(int id, string name, SourceType type)
        {
            this.id = id;
            this.name = name;
            this.type = type;
        }
    }
}
