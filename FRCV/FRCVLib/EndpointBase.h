#pragma once
#include <vector>
#include <memory>
#include <pthread.h>

class Logger;
class FilterBase;
// the following two classes are for one of the endpoint types (WebRTC)
//class Renderer;
//class DrawCommand;
class FilterAnalysis;

class EndpointBase
{
public:
	EndpointBase(std::shared_ptr<Logger> logger);
	void Toggle(bool wantedState);
	bool GetRunningState() const;
	void AddFilter(std::shared_ptr<FilterBase> filter);
protected:
	virtual void ProcessFrame(std::vector<FilterAnalysis> analysisVector) = 0;
private:
	std::shared_ptr<Logger> m_Logger;
	std::vector<std::shared_ptr<FilterBase>> m_Filters;
	
	// threading
	static void* ThreadEntry(void* context);
	void ThreadFunc();
	pthread_t m_Thread;
	bool m_ThreadRunning;
	bool m_ThreadWantedState;
};

