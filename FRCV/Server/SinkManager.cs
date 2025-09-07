using Swan;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Channels;
using System.Threading.Tasks;

namespace Server
{
    public class SinkManager
    {
        public static SinkManager Instance { get; } = new SinkManager();

        private string currentResults;
        private Thread thread;
        private List<Channel<string>> channels;
        private List<Sink> sinks;
        private bool isRunning;

        private SinkManager()
        {
            thread = new Thread(ThreadProc);

            sinks = new List<Sink>();
            channels = new List<Channel<string>>();
            isRunning = false;
        }

        public string GetResults()
        {
            return currentResults;
        }

        private void ThreadProc()
        {
            // This method will run in a separate thread to manage sinks
            while (isRunning)
            {
                updateResults();
                foreach (var channel in channels)
                {
                    channel.Writer.TryWrite(currentResults);
                }
                // Logic to manage sinks, e.g., checking for new data, processing it, etc.
                // This could involve reading from channels and updating sinks accordingly.
            }
        }

        public Sink GetSinkById(int id)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == id)
                {
                    return sink;
                }
            }

            return null; // or throw an exception if preferred
        }

        public int[] getAllSinkIds()
        {
            List<int> ids = new List<int>();
            foreach (var sink in sinks)
            {
                ids.Add(sink.Id);
            }

            return ids.ToArray();
        }

        public void SetSinkName(int sinkId, string dstName)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == sinkId)
                {
                    sink.Name = dstName;
                    DB.Instance.Save(); // Save changes to the database
                    break;
                }
            }
        }

        public void DeleteSink(int sinkId)
        {
            sinks.RemoveAll(sink => sink.Id == sinkId);
            DB.Instance.Save(); // Save changes to the database
        }

        public int AddSink(string name, string type, int? id = null)
        {
            if (id.HasValue)
            {
                switch (type)
                {
                    case "apritlag":
                        id = ManagerWrapper.Instance.CreateApriltagSink(id.Value);
                        sinks.Add(new Sink(id.Value, name, SinkType.ApriltagSink));
                        break;
                    case "objectdetectionsink":
                        id = ManagerWrapper.Instance.CreateObjectDetectionSink(id.Value);
                        sinks.Add(new Sink(id.Value, name, SinkType.ObjectDetectionSink));
                        break;
                }
                return id.GetValueOrDefault(-1);
            }
            else
            {
                switch (type)
                {
                    case "apriltag":
                        id = ManagerWrapper.Instance.CreateApriltagSink();
                        sinks.Add(new Sink(id.Value, name, SinkType.ApriltagSink));
                        break;
                    case "ObjectDetectionSink":
                        id = ManagerWrapper.Instance.CreateObjectDetectionSink();
                        sinks.Add(new Sink(id.Value, name, SinkType.ObjectDetectionSink));
                        break;
                }

                DB.Instance.Save(); // Save changes to the database
                return id.GetValueOrDefault(-1);
            }
        }

        public Channel<string> createResultChannel()
        {
            Channel<string> channel = Channel.CreateUnbounded<string>();

            channels.Add(channel);

            return channel;
        }

        // update results
        private void updateResults()
        {
            string[] results = getAllSinkIds().Select(id => ManagerWrapper.Instance.GetSinkResult(id)).ToArray();
            currentResults = JsonSerializer.Serialize(results);
            // Logic to update results in sinks
            // This could involve iterating through sinks and updating their state based on the data received.
        }

        // start the sink manager thread
        public void EnableManagerThread()
        {
            if (!isRunning)
            {
                isRunning = true;
                thread.Start();
            }
        }

        // stop the sink manager thread
        public void DisableManagerThread()
        {
            if (thread.IsAlive)
            {
                isRunning = false;
                thread.Join();
            }
        }

        // stop sink by id
        public void DisableSinkById(int id)
        {
            Source source = null;
            foreach (var sink in sinks)
            {
                if (sink.Id == id)
                {
                    if (sink.Source != null)
                    {
                        source = sink.Source;
                    }

                    ManagerWrapper.Instance.StopSinkById(id);
                    break;
                }
            }

            if (source != null)
            {
                bool isSourceUsed = false;
                foreach (var s in sinks)
                {
                    if (s.Source != null && s.Source.Id == source.Id)
                    {
                        isSourceUsed = true;
                        break;
                    }
                }

                if (!isSourceUsed)
                {
                    SourceManager.Instance.DisableSourceById(source.Id);
                }
            }
            // Logic to stop a sink by its ID
            // This could involve finding the sink in the sinks list and stopping it.
        }

        // start sink by id
        public void EnableSinkById(int id)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == id)
                {
                    if (sink.Source != null)
                    {
                        SourceManager.Instance.EnableSourceById(sink.Source.Id);
                    }

                    ManagerWrapper.Instance.StartSinkById(id);
                    return;
                }
            }
        }

        public void EnableAllSinks()
        {
            foreach (var sink in sinks)
            {
                if (sink.Source != null)
                {
                    SourceManager.Instance.EnableSourceById(sink.Source.Id);
                }

                ManagerWrapper.Instance.StartSinkById(sink.Id);
            }
        }

        public void UnbindSourceFromSink(int sinkId, int? sourceId = null)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == sinkId)
                {
                    ManagerWrapper.Instance.UnbindSourceFromSink(sinkId);
                    sink.Source = null; // Unbind the source
                    DB.Instance.Save(); // Save changes to the database
                    break;
                }
            }
        }

        public void BindSourceToSink(int sinkId, int sourceId)
        {
            foreach(var sink in sinks)
            {
                if(sink.Id == sinkId)
                {
                    sink.Source = SourceManager.Instance.GetSourceById(sourceId);
                    ManagerWrapper.Instance.BindSourceToSink(sourceId, sinkId);
                    SourceManager.Instance.EnableSourceById(sink.Source.Id);
                    DB.Instance.Save(); // Save changes to the database
                    break;
                }
            }
        }
    }
}
