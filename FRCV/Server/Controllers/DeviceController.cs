using EmbedIO;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Controllers
{
    internal class DeviceController : WebApiController
    {
        [Route(HttpVerbs.Get, "/device/cpuUsage")]
        public Task<int> GetDeviceCPUUsage() 
        { 
            return Task.FromResult(ManagerWrapper.Instance.GetCPUUsage()); 
        }

        [Route(HttpVerbs.Get, "/device/ramUsage")]
        public Task<int> GetDeviceRamUsage()
        {
            return Task.FromResult(ManagerWrapper.Instance.GetMemoryUsageBytes());
        }

        [Route(HttpVerbs.Get, "/device/diskUsage")]
        public Task<int> GetDeviceDiskUsage()
        {
            return Task.FromResult(ManagerWrapper.Instance.GetDiskUsage());
        }
    }
}
