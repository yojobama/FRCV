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

    public class Source
    {
        private int id { get; set; }
        private string name { get; set; }
        private SourceType type { get; set; }

        public int Id
        {
            get => id; set => id = value;
        }
        
        public string Name
        {
            get => name;
            set => name = value;
        }
        
        public Source(int id, string name, SourceType type)
        {
            this.id = id;
            this.name = name;
            this.type = type;
        }
    }
}
