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
        int id { get; set; }
        string name { get; set; }
        SourceType type { get; set; }

        public Source(int id, string name, SourceType type)
        {
            this.id = id;
            this.name = name;
            this.type = type;
        }
    }
}
