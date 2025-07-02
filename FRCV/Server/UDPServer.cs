using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Channels;
using System.Threading.Tasks;

namespace Server
{
    internal class UDPServer
    {
        Channel<string> channel;
        public UDPServer(Channel<string> channel)
        {
            this.channel = channel;
            // Initialize the UDP server
            // This could include setting up sockets, binding to ports, etc.
        }
    }
}
