using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers
{
    internal class DeviceController : ControllerBase
    {
        [HttpGet("device/cpuUsage")]
        public Task<int> GetDeviceCPUUsage() 
        { 
            return Task.FromResult((int)LinuxResourceMonitor.Instance.GetLatestResourceInfo().CpuUsagePercent); 
        }

        [HttpGet("device/ramUsage")]
        public Task<int> GetDeviceRamUsage()
        {
            return Task.FromResult((int)LinuxResourceMonitor.Instance.GetLatestResourceInfo().UsedMemoryMB);
        }

        [HttpGet("device/diskUsage")]
        public Task<int> GetDeviceDiskUsage()
        {
            return Task.FromResult((int)LinuxResourceMonitor.Instance.GetLatestResourceInfo().RootDiskUsage.UsedPercent);
        }

        // ROADMAP.md Phase 8e (match view): Manager::GetCpuTemperature() has existed natively
        // since the SystemMonitor work but was never exposed over REST - a straight passthrough,
        // not a cached value like the other three (LinuxResourceMonitor's own background sampler
        // doesn't track temperature), so this makes one native call per request.
        [HttpGet("device/temperature")]
        public Task<int> GetDeviceTemperature()
        {
            return Task.FromResult(ManagerWrapper.Instance.GetCpuTemperature());
        }
    }
}
