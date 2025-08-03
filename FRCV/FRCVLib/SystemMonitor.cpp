#include "SystemMonitor.h"

#include <thread>
#include <chrono>

SystemMonitor::SystemMonitor(int timeout)
{
	m_TimeoutMilliseconds = timeout;
}

SystemMonitor::~SystemMonitor()
{
}

void SystemMonitor::StartMonitoring()
{
    m_ThreadWantedAlive = true;
    pthread_create(&m_MonitorThread, nullptr, s_StartMonitorThread, this);
}

void SystemMonitor::StopMonitoring()
{
    m_ThreadWantedAlive = false;
    if (m_MonitorThread) {
        pthread_join(m_MonitorThread, nullptr); // Wait for the thread to finish
        m_MonitorThread = 0; // Reset the thread ID
	}
}

void* SystemMonitor::s_StartMonitorThread(void* arg)
{
	static_cast<SystemMonitor*>(arg)->m_MonitorThreadLoop();
    return nullptr;
}

void SystemMonitor::m_MonitorThreadLoop()
{
    while (m_ThreadWantedAlive)
    {
        // Monitor system resources
        {
            std::lock_guard<std::mutex> guard(m_CPUUsageMutex);
            m_cpuUsage = m_GetCPUUsage(m_ReadCPUData(), m_ReadCPUData());
        }
        {
            std::lock_guard<std::mutex> guard(m_CPUTemperatureMutex);
            m_cpu_Temperature = m_GetThermalZoneTemperature(m_FindThermalZoneIndex());
        }
        {
            std::lock_guard<std::mutex> guard(m_DiskMutex);
            m_diskUsage = m_GetDiskUsage("/");
        }
        {
            std::lock_guard<std::mutex> guard(m_RAMUsageMutex);
            m_ramUsage = m_ReadMemoryData().get_memory_usage();
        }
		// sleep for the specified timeout
		std::this_thread::sleep_for(std::chrono::milliseconds(m_TimeoutMilliseconds));
    }
}

CPU_STATS SystemMonitor::m_ReadCPUData()
{
    CPU_STATS result;
    std::ifstream proc_stat("/proc/stat");

    if (proc_stat.good())
    {
        std::string line;
        getline(proc_stat, line);

        unsigned int* stats_p = (unsigned int*)&result;
        std::stringstream iss(line);
        std::string cpu;
        iss >> cpu;
        while (iss >> *stats_p)
        {
            stats_p++;
        };
    }

    proc_stat.close();

    return result;
}

int SystemMonitor::m_GetVal(const std::string& target, const std::string& content)
{
    int result = -1;

	std::size_t start = content.find(target);
    if (start != std::string::npos) {
		int begine = start + target.length();
		std::size_t end = content.find("kB", begine);
		std::string substr = content.substr(begine, end - begine);
		result = std::stoi(substr);
    }

    return result;
}

float SystemMonitor::m_GetCPUUsage(const CPU_STATS& first, const CPU_STATS& second)
{
    const float active_time = static_cast<float>(second.get_total_active() - first.get_total_active());
    const float idle_time = static_cast<float>(second.get_total_idle() - first.get_total_idle());
    const float total_time = active_time + idle_time;
    return active_time / total_time;
}

float SystemMonitor::m_GetDiskUsage(const std::string& disk)
{
    struct statvfs diskData;

    statvfs(disk.c_str(), &diskData);

    auto total = diskData.f_blocks;
    auto free = diskData.f_bfree;
    auto diff = total - free;

    float result = static_cast<float>(diff) / total;

    return result;
}

int SystemMonitor::m_FindThermalZoneIndex()
{
    int result = 0;
    bool stop = false;
    // 20 must stop anyway
    for (int i = 0; !stop && i < 20; ++i) {
        std::ifstream thermal_file("/sys/class/thermal/thermal_zone" + std::to_string(i) + "/type");

        if (thermal_file.good())
        {
            std::string line;
            getline(thermal_file, line);

            if (line.compare("x86_pkg_temp") == 0) {
                result = i;
                stop = true;
            }

        }
        else {
            stop = true;
        }

        thermal_file.close();
    }
    return result;
}

int SystemMonitor::m_GetThermalZoneTemperature(int index)
{
    int result = -1;
    // TODO: fix
    /*std::ifstream thermal_file("/sys/class/thermal/thermal_zone" + std::to_string(index) + "/temp");

    if (thermal_file.good())
    {
        std::string line;
        getline(thermal_file, line);

        std::stringstream iss(line);
        iss >> result;
    }
    else {
        throw std::invalid_argument(std::to_string(index) + " doesn't refer to a valid thermal zone.");
    }

    thermal_file.close();*/

    return result;
}

MEMORY_STATS SystemMonitor::m_ReadMemoryData()
{
    MEMORY_STATS result;
    std::ifstream proc_meminfo("/proc/meminfo");

    if (proc_meminfo.good())
    {
        std::string content((std::istreambuf_iterator<char>(proc_meminfo)),
            std::istreambuf_iterator<char>());

        result.total_memory = m_GetVal("MemTotal:", content);
        result.total_swap = m_GetVal("SwapTotal:", content);
        result.free_swap = m_GetVal("SwapFree:", content);
        result.available_memory = m_GetVal("MemAvailable:", content);

    }

    proc_meminfo.close();

    return result;
}

// get ram usage in megabytes
int SystemMonitor::GetRAMUsage()
{
    std::lock_guard<std::mutex> guard(m_RAMUsageMutex);
	return static_cast<int>(m_ramUsage * 1000); // Convert to megabytes
}

int SystemMonitor::GetCPUTemperature()
{
    std::lock_guard<std::mutex> guard(m_CPUTemperatureMutex);
    return m_cpu_Temperature / 1000; // Convert from millidegrees to degrees
}

float SystemMonitor::GetCPUUsage()
{
    std::lock_guard<std::mutex> guard(m_CPUUsageMutex);
    return m_cpuUsage * 100; // Convert to percentage
}

float SystemMonitor::GetDiskUsage()
{
    std::lock_guard<std::mutex> guard(m_DiskMutex);
    return m_diskUsage * 100; // Convert to percentage
}