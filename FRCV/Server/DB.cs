using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public class DB
    {
        public static DB Instance { get; } = new DB("data.json");

        string jsonPath;
        
        
        private DB(string jsonPath)
        {
            this.jsonPath = jsonPath;
        }

        public void RemoveSink(Sink sink)
        {

        }
        public void AddSink(Sink sink)
        {

        }


        public void RemoveSource()
        {

        }
        public void AddSource()
        {

        }
    }
}
