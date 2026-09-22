#pragma once
#include <vector>

// RK3588 (and other big.LITTLE ARM SoCs) run at very different sustained throughput between
// their performance (A76) and efficiency (A55) cores - a detector/encoder thread scheduled onto
// an efficiency core competes with capture/UI/GC threads for a fraction of the throughput a
// performance core offers. Rather than hardcoding "cores 4-7" (true for the reference Orange Pi
// 5/5+ image this project targets, but not guaranteed for every RK3588 board's devicetree, and
// meaningless on non-big.LITTLE hardware), this detects performance cores at runtime by reading
// each online CPU's max scaling frequency from sysfs and pinning to whichever frequency tier is
// highest. On non-Linux builds, or any system that isn't big.LITTLE (all cores report the same
// max frequency, or no cpufreq info exists at all), this is a no-op - the OS scheduler's own
// default behaviour is left alone rather than pinning to a guess.
namespace CpuAffinity
{
	// Cached after the first call - core topology doesn't change at runtime. Empty means
	// "not big.LITTLE (or couldn't tell)" - callers should treat that as "don't pin".
	const std::vector<int>& GetPerformanceCoreIds();

	// Pins the CALLING thread to the performance core set (no-op if none were detected, or on
	// non-Linux platforms). Intended for the heavy pipeline threads - see
	// ISource::SourceThreadProc/ISink::ProcessingThreadLoop - not the lightweight SystemMonitor
	// polling thread, which has no reason to compete for the fast cores.
	void PinCurrentThreadToPerformanceCores();
}
