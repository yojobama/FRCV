using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    internal class SourceManager
    {
        private List<Source> sources;
        private readonly Manager manager;

        public SourceManager(ref Manager manager)
        {
            this.manager = manager;
            // Initialize the source manager
            // This could include setting up data sources, initializing channels, etc.
        }

        public void DisableAllSources()
        {
            // Logic to stop all sources
            // This could involve iterating through a list of sources and stopping each one.
        }
        public void EnableAllSources()
        {
            // Logic to start all sources
            // This could involve iterating through a list of sources and starting each one.
        }

        public void DisableSourceById(int id)
        {
            // Logic to disable a source by its ID
            // This could involve finding the source in a list and disabling it.
        }
        public void EnableSourceById(int id)
        {
            // Logic to enable a source by its ID
            // This could involve finding the source in a list and enabling it.
        }
    }
}
