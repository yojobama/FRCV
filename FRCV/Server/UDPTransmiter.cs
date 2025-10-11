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
            const int MAX_UDP_SIZE = 1400; // Safe size below MTU
            
            try
            {
                byte[] testMsg = Encoding.UTF8.GetBytes("starting udp transmissions");
                udpClient.Send(testMsg, testMsg.Length, remoteEP);

                while (threadWantedAlive)
                {
                    if (channel.Reader.TryRead(out string? message))
                    {
                        byte[] msg = Encoding.UTF8.GetBytes(message);
                        
                        // Fragment large messages
                        if (msg.Length > MAX_UDP_SIZE)
                        {
                            int totalChunks = (msg.Length + MAX_UDP_SIZE - 1) / MAX_UDP_SIZE;
                            for (int i = 0; i < totalChunks; i++)
                            {
                                int offset = i * MAX_UDP_SIZE;
                                int chunkSize = Math.Min(MAX_UDP_SIZE, msg.Length - offset);
                                
                                // Add header: [chunkIndex/totalChunks]
                                string header = $"[{i}/{totalChunks}]";
                                byte[] headerBytes = Encoding.UTF8.GetBytes(header);
                                byte[] chunk = new byte[headerBytes.Length + chunkSize];
                                
                                Array.Copy(headerBytes, 0, chunk, 0, headerBytes.Length);
                                Array.Copy(msg, offset, chunk, headerBytes.Length, chunkSize);
                                
                                udpClient.Send(chunk, chunk.Length, remoteEP);
                                Thread.Sleep(1); // Small delay between fragments
                            }
                        }
                        else
                        {
                            udpClient.Send(msg, msg.Length, remoteEP);
                        }
                    }
                    else
                    {
                        Thread.Sleep(10);
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