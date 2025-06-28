using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Channels;
using System.Threading.Tasks;

namespace ConsoleApp1
{
    internal class UDPServer
    {
        Thread m_thread;
        Channel<string> m_channel;
        Socket[] m_sockets;
        bool m_running = true;

        public UDPServer(Channel<string> channel, string[] adresses)
        {
            m_channel = channel;
            m_thread = new Thread(Run);
            m_sockets = new Socket[adresses.Length];
            for(int i = 0; i < adresses.Length; i++)
            {
                var socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
                socket.Bind(new System.Net.IPEndPoint(System.Net.IPAddress.Parse(adresses[i]), 11000));
                m_sockets[i] = socket;
            }
        }

        public void Start()
        {
            m_running = true;
            
            m_thread.Start();
        }

        public void Stop()
        {
            m_running = false;
            m_thread.Join();
            foreach (var socket in m_sockets)
            {
                socket.Close();
            }
        }

        void Run()
        {
            while(m_running)
            {
                string message = m_channel.Reader.ReadAsync().Result;
                foreach (var socket in m_sockets)
                {
                    try
                    {
                        byte[] data = Encoding.UTF8.GetBytes(message);
                        socket.Send(data);
                    }
                    catch (SocketException ex)
                    {
                        Console.WriteLine($"Socket error: {ex.Message}");
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Error sending message: {ex.Message}");
                    }
                }
            }
        }
    }
}
