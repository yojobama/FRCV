using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Channels;
using System.Threading;

namespace Server
{
    internal class UDPTransmiter
    {
        private readonly Channel<string> channel;
        private readonly Thread thread;
        private volatile bool threadWantedAlive;

        private readonly UdpClient udpClient;
        private readonly IPEndPoint remoteEP;
        private readonly string hostName;
        private readonly int port;
        private readonly Logger logger;

        public string HostName => hostName;

        public UDPTransmiter(Channel<string> channel, string hostName, int port)
        {
            this.channel = channel;
            this.hostName = hostName;
            this.port = port;

            // Create a connectionless UDP client
            udpClient = new UdpClient();
            remoteEP = new IPEndPoint(IPAddress.Parse(hostName), port);

            thread = new Thread(ThreadProc);
            logger = new Logger("UDPTransmiterLog.txt");
        }

        public void Enable()
        {
            logger.EnterLog("UDPTransmiter Enable called");
            threadWantedAlive = true;
            thread.Start();
            logger.EnterLog("UDPTransmiter thread started");
        }

        public void Disable()
        {
            logger.EnterLog("UDPTransmiter Disable called");
            threadWantedAlive = false;
            thread.Join();
            logger.EnterLog("UDPTransmiter thread stopped");
        }

        private void ThreadProc()
        {
            try
            {
                byte[] testMsg = Encoding.UTF8.GetBytes("starting udp transmissions");
                udpClient.Send(testMsg, testMsg.Length, remoteEP);

                while (threadWantedAlive)
                {
                    if (channel.Reader.TryRead(out string? message))
                    {
                        byte[] msg = Encoding.UTF8.GetBytes(message);
                        udpClient.Send(msg, msg.Length, remoteEP);
                    }
                    else
                    {
                        Thread.Sleep(10); // Prevent tight loop if nothing to read
                    }
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"UDPTransmiter error: {ex.Message}");
            }
            finally
            {
                udpClient.Dispose();
            }
        }
    }
}