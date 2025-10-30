#include "FilterBase.h"

void FilterBase::ThreadFunc()
{
	m_ThreadRunning = true;
	while (m_ThreadShouldRun) 
	{

	}
	m_ThreadRunning = false;
}
