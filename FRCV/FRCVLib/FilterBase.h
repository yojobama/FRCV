#pragma once
#include <vector>
#include <string>
#include <pthread.h>
#include <memory>
#include <mutex>

class SourceBase;
class DrawCommand;
class FilterAnalysis;
class Logger;

class FilterBase
{
public:
	FilterBase(Logger* logger, int maxSources);
	virtual ~FilterBase(); // TODO: consider what to do with the wee lad

	void ToggleDrawCommands(bool drawCommandsRequired);
	void AddSource(std::shared_ptr<SourceBase> source);

	std::shared_ptr<FilterAnalysis> GetAnalysis();
protected:
	std::vector<std::shared_ptr<SourceBase>> m_Sources;
	virtual FilterAnalysis Process() = 0;
	virtual std::vector<DrawCommand> CreateDrawCommands(std::shared_ptr<FilterAnalysis> analysis) = 0;
private:
	int m_MaxSources;
	bool m_DrawCommandsRequired;
	// threading
	static void* ThreadEntry(void* context);
	void ThreadFunc();
	pthread_t m_Thread;
	bool m_ThreadShouldRun;
	bool m_ThreadRunning;
	std::mutex m_Mutex;
};

