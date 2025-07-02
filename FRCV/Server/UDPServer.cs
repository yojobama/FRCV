using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Channels;
using System.Threading.Tasks;

namespace Server
{
    internal class UDPServer
    {
        Channel<string> channel;
        Thread thread;
        bool threadWantedAlive;

        UdpClient udpClient;
        
        public UDPServer(Channel<string> channel, string hostName, int port)
        {
            this.channel = channel;
            // Initialize the UDP server
            // This could include setting up sockets, binding to ports, etc.
            thread = new Thread(ThreadProc);
            threadWantedAlive = false;

            udpClient = new UdpClient(hostName, port);
        }

        public void Start()
        {
            threadWantedAlive = true;
            thread.Start();
        }

        public void Stop() 
        {
            threadWantedAlive = false;
            thread.Join();
        }

        private void ThreadProc()
        {
            while (threadWantedAlive)
            {
                byte[] msg = Encoding.UTF8.GetBytes(channel.Reader.ToString()!);
                udpClient.Send(msg, msg.Length);
            }
        }
    }
}
