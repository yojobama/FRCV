using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Channels;
using System.Threading.Tasks;

namespace Server
{
    internal class SinkManager
    {
        private string currentResults;
        private Manager manager;
        private Thread thread;
        private List<Channel<string>> channels;
        private List<Sink> sinks;
        private readonly SourceManager sourceManager;
        private bool isRunning;

        public SinkManager(ref SourceManager sourceManager, ref Manager manager)
        {
            this.manager = manager;
            this.sourceManager = sourceManager;
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
                    break;
                }
            }
        }

        public void AddSink(SinkType type)
        {
            int id;

            switch (type)
            {
                case SinkType.ApriltagSink:
                    id = manager.createApriltagSink();
                    sinks.Add(new Sink(id, "Apriltag Sink", SinkType.ApriltagSink));
                    break;
                case SinkType.ObjectDetectionSink:
                    id = manager.createObjectDetectionSink();
                    sinks.Add(new Sink(id, "Object Detection Sink", SinkType.ObjectDetectionSink));
                    break;
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
            currentResults = manager.getAllSinkResults();
            // Logic to update results in sinks
            // This could involve iterating through sinks and updating their state based on the data received.
        }

        // start the sink manager thread
        public void EnableManagerThread()
        {
            isRunning = true;
            thread.Start();
        }

        // stop the sink manager thread
        public void DisableManagerThread()
        {
            isRunning = false;
            if (thread.IsAlive)
            {
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

                    manager.stopSinkById(id);
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
                    sourceManager.DisableSourceById(source.Id);
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
                        sourceManager.EnableSourceById(sink.Source.Id);
                    }

                    manager.startSinkById(id);
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
                    sourceManager.EnableSourceById(sink.Source.Id);
                }

                manager.startSinkById(sink.Id);
            }
        }

        public void UnbindSourceFromSink(int sinkId, int sourceId)
        {
            foreach (var sink in sinks)
            {
                if (sink.Id == sinkId)
                {
                    sink.Source = null;
                    break;
                }
            }
            // Logic to unbind a source from a sink
            // This could involve setting the source property of the sink to null or removing the association.
        }
    }
}
