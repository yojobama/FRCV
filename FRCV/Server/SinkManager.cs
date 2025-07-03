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
                if (sink.id == id)
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
                ids.Add(sink.id);
            }
            return ids.ToArray();
        }

        public void SetSinkName(string name)
        {

        }

        public void AddSink(SinkType type)
        {
            
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
        public void StartManagerThread()
        {
            isRunning = true;
            thread.Start();
        }
        // stop the sink manager thread
        public void StopManagerThread()
        {
            isRunning = false;
            if (thread.IsAlive)
            {
                thread.Join();
            }
        }

        // stop sink by id
        public void StopSinkById(int id)
        {
            Source source = null;
            foreach (var sink in sinks)
            {
                if (sink.id == id)
                {
                    if (sink.source != null)
                    {
                        source = sink.source;
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
                    if (s.source != null && s.source.id == source.id)
                    {
                        isSourceUsed = true;
                        break;
                    }
                }
                if (!isSourceUsed)
                {
                    sourceManager.DisableSourceById(source.id);
                }
            }
            // Logic to stop a sink by its ID
            // This could involve finding the sink in the sinks list and stopping it.
        }
        // start sink by id
        public void StartSinkById(int id)
        {
            foreach (var sink in sinks)
            {
                if (sink.id == id)
                {
                    if (sink.source != null)
                    {
                        sourceManager.EnableSourceById(sink.source.id);
                    }
                    manager.startSinkById(id);
                    return;
                }
            }
        }
    }
}
