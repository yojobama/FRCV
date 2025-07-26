#include "ISink.h"
#include "Frame.h"

ISink::ISink(Logger* p_Logger) : m_Logger(p_Logger), m_Source(nullptr) {
    if (m_Logger) m_Logger->EnterLog("ISink constructed");
}

std::string ISink::GetStatus() {
    return "";
}

void* ISink::SinkThreadStart(void* p_Reference)
{
    ((ISink*)p_Reference)->SinkThreadProc();
    return nullptr;
}

void ISink::ChangeThreadStatus(bool threadWantedAlive)
{
    if (threadWantedAlive) {
        m_ShouldTerminate = false;
        pthread_create(&m_Thread, NULL, SinkThreadStart, this);
    }
    else {
        if (m_Thread) {
            m_ShouldTerminate = true;
            pthread_join(m_Thread, NULL);
        }
    }
}

void ISink::SinkThreadProc()
{
    while (!m_ShouldTerminate) {
        // do stuff
        ProcessFrame();
    }
    m_ShouldTerminate = false;
	pthread_exit(NULL);
}

string ISink::GetCurrentResults()
{
    return m_Results;
}

bool ISink::BindSource(ISource* p_Source) {
    if (m_Logger) m_Logger->EnterLog("ISink::BindSource called");
    if (p_Source == nullptr) {
        m_Logger->EnterLog(LogLevel::Error, "ISink::BindSource: Source is null");
        return false;
    }
    this->m_Source = p_Source;
    return true;
}

bool ISink::UnbindSource() {
    if (m_Logger) m_Logger->EnterLog("ISink::UnbindSource called");
    if (m_Source == nullptr) {
        m_Logger->EnterLog(LogLevel::Error, "ISink::UnbindSource: Source is already null");
        return false;
    }
    m_Source = nullptr;
    return true;
}