#include "EndpointBase.h"

#include "Logger.h"

EndpointBase::EndpointBase(std::shared_ptr<Logger> logger)
{
	m_Logger = logger;
}

void EndpointBase::Toggle(bool wantedState)
{
	if (wantedState) {
		m_ThreadWantedState = true;
		pthread_create(&m_Thread, nullptr, EndpointBase::ThreadEntry, this);
	}
	else if(m_Thread) {
		m_ThreadWantedState = false;
		pthread_join(m_Thread, nullptr);
	}
}

bool EndpointBase::GetRunningState() const
{
	return m_ThreadRunning;
}

void EndpointBase::AddFilter(std::shared_ptr<FilterBase> filter)
{
	m_Filters.push_back(filter);
}

void* EndpointBase::ThreadEntry(void* p_Reference)
{
	((EndpointBase*)p_Reference)->ThreadFunc();
	return nullptr;
}

void EndpointBase::ThreadFunc()
{
	m_ThreadRunning = true;
	while (m_ThreadWantedState) {

	}
	m_ThreadRunning = false;
	pthread_exit(nullptr);
}
