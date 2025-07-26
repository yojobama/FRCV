#pragma once
#include <pthread.h>
#include <string>

#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/statvfs.h>
#include <mutex>

struct CPU_STATS {
    int user;
    int nice;
    int system;
    int idle;
    int iowait;
    int irq;
    int softirq;
    int steal;
    int guest;
    int guest_nice;

    int get_total_idle()
        const {
        return idle + iowait;
    }

    int get_total_active()
        const {
        return user + nice + system + irq + softirq + steal + guest + guest_nice;
    }
};

struct MEMORY_STATS {
    int total_memory;
    int available_memory;
    int total_swap;
    int free_swap;

    float get_memory_usage() {
        const float result = static_cast<float>(total_memory - available_memory) / total_memory;
        return result;
    }

    float get_swap_usage() {
        const float result = static_cast<float>(total_swap - free_swap) / total_swap;
        return result;
    }
};


class SystemMonitor
{
public:
	SystemMonitor();
	~SystemMonitor();
private:
	// thread management
    pthread_t monitorThread; 
	static void* s_StartMonitorThread(void* arg);
	void m_MonitorThreadLoop();

	// methods for reading system data
    CPU_STATS m_ReadCPUData();
	int m_GetVal(const std::string& target, const std::string& content);
	float m_GetCPUUsage(const CPU_STATS& first, const CPU_STATS& second);
	float m_GetDiskUsage(const std::string& disk);
	int m_FindThermalZoneIndex();
	int m_GetThermalZoneTemperature(int index);
    
	// variables to store system data
    float m_cpuUsage;
    float m_diskUsage;
	int m_ramUsage;
    int m_cpu_Temperature;

	// mutex for thread safety
	std::mutex m_CPUUsageMutex;
	std::mutex m_DiskMutex;
	std::mutex m_CPUTemperatureMutex;
};

