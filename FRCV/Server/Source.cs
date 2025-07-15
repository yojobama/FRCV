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
        private int id;
        private string name;
        private SourceType type;

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
        
        public Source(int id, string name, SourceType type)
        {
            this.id = id;
            this.name = name;
            this.type = type;
        }
    }
}
