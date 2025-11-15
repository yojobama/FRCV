#include "FilterBase.h"
#include "FilterAnalysis.h"
#include "Logger.h"
#include "DrawCommands.h"

FilterBase::FilterBase(std::shared_ptr<Logger> logger, int maxSources)
{
	m_Logger = logger;
	m_MaxSources = maxSources;
}

// TODO: Implement the descructor
FilterBase::~FilterBase()
{
}

void FilterBase::ToggleDrawCommands(bool drawCommandsRequired)
{
	m_DrawCommandsRequired = drawCommandsRequired;
}

bool FilterBase::GetThreadRunning()
{
	return m_ThreadRunning;
}

void FilterBase::AddSource(std::shared_ptr<SourceBase> source)
{
	if (m_Sources.size() < m_MaxSources) {
		m_SourcesMutex.lock();
		m_Sources.push_back(source);
		m_SourcesMutex.unlock();
	}
}

std::shared_ptr<FilterAnalysis> FilterBase::GetAnalysis()
{
	std::lock_guard<std::mutex> lock(m_LatestAnalysisMutex);
	return m_LatestAnalysis;
}

void* FilterBase::ThreadEntry(void* p_Reference)
{
	((FilterBase*)p_Reference)->ThreadProcess();
	return nullptr;
}

void FilterBase::ThreadProcess()
{
	m_ThreadRunning = true;
	std::shared_ptr<FilterAnalysis> analysis;
	while (m_ThreadShouldRun) {
		analysis = std::make_shared<FilterAnalysis>(Process());
		if (m_DrawCommandsRequired) analysis->SetDrawCommands(CreateDrawCommands(analysis));
		{
			std::lock_guard<std::mutex> lock(m_LatestAnalysisMutex);
			m_LatestAnalysis = analysis;
		}
	}
	m_ThreadRunning = false;
	pthread_exit(nullptr);
}
