#include "CpuAffinity.h"

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#include <fstream>
#include <map>
#include <thread>
#endif

namespace CpuAffinity
{

#ifdef __linux__
static std::vector<int> DetectPerformanceCores()
{
	unsigned int cpuCount = std::thread::hardware_concurrency();
	// keyed by max scaling frequency (kHz) so cores sharing a frequency tier group together
	// regardless of their physical index - std::map keeps this sorted ascending by key.
	std::map<long, std::vector<int>> coresByMaxFreq;
	for (unsigned int cpu = 0; cpu < cpuCount; ++cpu) {
		std::ifstream freqFile("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpufreq/cpuinfo_max_freq");
		long maxFreq = 0;
		if (freqFile.is_open()) freqFile >> maxFreq;
		if (maxFreq > 0) coresByMaxFreq[maxFreq].push_back(static_cast<int>(cpu));
	}

	// a single frequency tier means either uniform (non-big.LITTLE) hardware or that cpufreq
	// info wasn't readable at all - either way there's no "performance core set" to pin to.
	if (coresByMaxFreq.size() < 2) return {};

	// the last entry (highest key) is the fastest tier - on the reference RK3588 Orange Pi 5
	// image this resolves to the 4 Cortex-A76 cores, without hardcoding their index.
	return coresByMaxFreq.rbegin()->second;
}
#endif

const std::vector<int>& GetPerformanceCoreIds()
{
#ifdef __linux__
	static const std::vector<int> cores = DetectPerformanceCores();
#else
	static const std::vector<int> cores;
#endif
	return cores;
}

void PinCurrentThreadToPerformanceCores()
{
#ifdef __linux__
	const std::vector<int>& cores = GetPerformanceCoreIds();
	if (cores.empty()) return;

	cpu_set_t cpuSet;
	CPU_ZERO(&cpuSet);
	for (int core : cores) CPU_SET(core, &cpuSet);
	// best-effort - a failure here (e.g. cgroup/affinity restrictions in a container) just
	// leaves the thread on the OS scheduler's default placement, not worth failing over.
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuSet);
#endif
}

} // namespace CpuAffinity
