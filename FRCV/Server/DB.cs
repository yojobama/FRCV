using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace Server
{
    public class DB
    {
        private class DBData
        {
            public List<Sink> Sinks { get; set; }
            public List<Source> Sources { get; set; }
        }

        public static DB Instance { get; } = new DB("data.json");

        private string jsonPath;
        private List<Sink> sinks;
        private List<Source> sources;
        private readonly Logger logger;

        private DB(string jsonPath)
        {
            this.jsonPath = jsonPath;
            sinks = new List<Sink>();
            sources = new List<Source>();
            logger = new Logger("DBLog.txt"); // Initialize logger with a log file
        }

        public string GetJson()
        {
            var dbData = new DBData
            {
                Sinks = sinks,
                Sources = sources
            };
            return JsonSerializer.Serialize(dbData, new JsonSerializerOptions { WriteIndented = true });
        }

        public void Load()
        {
            logger.enterLog("DB Load called");
            if (File.Exists(jsonPath))
            {
                string jsonData = File.ReadAllText(jsonPath);
                var dbData = JsonSerializer.Deserialize<DBData>(jsonData);

                if (dbData != null)
                {
                    sinks = dbData.Sinks ?? new List<Sink>();
                    sources = dbData.Sources ?? new List<Source>();
                }

                foreach (var source in sources)
                {
                    switch(source.Type)
                    {
                        case SourceType.ImageFile:
                            SourceManager.Instance.initializeImageFileSource(source.FilePath, source.Name, source.Id);
                            break;
                        case SourceType.VideoFile:
                            SourceManager.Instance.InitializeVideoFileSource(source.FilePath, source.Fps ?? 30, source.Name, source.Id);
                            break;
                        case SourceType.Camera:
                            SourceManager.Instance.InitializeCameraSource(source.CameraHardwareInfo ,id: source.Id);
                            break;
                    }
                }
                foreach (var sink in sinks)
                {
                    SinkManager.Instance.AddSink(sink.Name, sink.Type.ToString());
                }

                logger.enterLog("DB loaded successfully from " + jsonPath);
            }
            else
            {
                logger.enterLog("DB file not found: " + jsonPath);
            }
        }

        public void Save()
        {
            logger.enterLog("DB Save called");
            List<Sink> sinks = new List<Sink>();
            List<Source> sources = new List<Source>();

            foreach (var sink in SinkManager.Instance.getAllSinkIds())
            {
                sinks.Add(SinkManager.Instance.GetSinkById(sink));
            }
            foreach (var source in SourceManager.Instance.GetAllSourceIds())
            {
                sources.Add(SourceManager.Instance.GetSourceById(source));
            }

            var dbData = new DBData
            {
                Sinks = sinks,
                Sources = sources
            };

            string jsonData = JsonSerializer.Serialize(dbData, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(jsonPath, jsonData);
            logger.enterLog("DB saved successfully to " + jsonPath);
        }

        public void RemoveSink(Sink sink)
        {
            sinks.Remove(sink);
            Save();
        }

        public void RemoveSource(Source source)
        {
            sources.Remove(source);
            Save();
        }

        public List<Sink> GetSinks()
        {
            return sinks;
        }

        public List<Source> GetSources()
        {
            return sources;
        }

        public void Verify()
        {
            foreach (var sink in sinks)
            {
                if (SinkManager.Instance.GetSinkById(sink.Id) == null)
                {
                    SinkManager.Instance.AddSink(sink.Name, sink.Type.ToString());
                }
            }

            foreach (var source in sources)
            {
                if (SourceManager.Instance.GetSourceById(source.Id) == null)
                {
                    SourceManager.Instance.InitializeCameraSource(new CameraHardwareInfo { name = source.Name, path = "" });
                }
            }
        }
    }
}
