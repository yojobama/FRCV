#pragma once
#include <vector>
#include <string>
#include <pthread.h>
#include <memory>
#include <mutex>

namespace DrawCommands
{
	class DrawCommandBase;
}

class SourceBase;
class FilterAnalysis;
class Logger;

class FilterBase
{
public:
	FilterBase(std::shared_ptr<Logger> logger, int maxSources);
	virtual ~FilterBase(); // TODO: consider what to do with the wee lad

	void ToggleDrawCommands(bool drawCommandsRequired);
	bool GetThreadRunning();

	void AddSource(std::shared_ptr<SourceBase> source);

	std::shared_ptr<FilterAnalysis> GetAnalysis();
protected:
	std::vector<std::shared_ptr<SourceBase>> m_Sources;
	virtual FilterAnalysis Process() = 0;
	virtual std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> CreateDrawCommands(std::shared_ptr<FilterAnalysis> analysis) = 0;
private:
	std::shared_ptr<FilterAnalysis> m_LatestAnalysis;
	std::shared_ptr<Logger> m_Logger;
	int m_MaxSources;
	bool m_DrawCommandsRequired;
	// threading
	static void* ThreadEntry(void* context);
	void ThreadProcess();
	pthread_t m_Thread;
	bool m_ThreadShouldRun;
	bool m_ThreadRunning;
	std::mutex m_SourcesMutex;
	std::mutex m_LatestAnalysisMutex;
};

