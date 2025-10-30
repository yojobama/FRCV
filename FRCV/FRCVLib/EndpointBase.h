#pragma once
#include <vector>
#include <memory>
#include <pthread.h>

class FilterBase;
// the following two classes are for one of the endpoint types (WebRTC)
//class Renderer;
//class DrawCommand;
class FilterAnalysis;

class EndpointBase
{
public:
	EndpointBase();
	void Toggle(bool wantedState);
	void AddFilter(std::shared_ptr<FilterBase> filter);
protected:
	virtual void ProcessFrame(std::vector<FilterAnalysis> analysisVector) = 0;
private:
	std::vector<std::shared_ptr<FilterBase>> m_Filters;
	
	// threading
	static void* ThreadEntry(void* context);
	void ThreadFunc();
	pthread_t m_Thread;
	bool m_ThreadRunning;
};

