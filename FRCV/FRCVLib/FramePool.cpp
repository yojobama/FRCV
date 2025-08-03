#include "FramePool.h"
#include "Frame.h"

// initializing the frame pool
FramePool::FramePool(Logger* p_Logger) : m_Logger(p_Logger) {
    if (m_Logger) m_Logger->EnterLog("FramePool constructed");
}

FramePool::~FramePool() {
    if (m_Logger) m_Logger->EnterLog("FramePool destructed");
}

int FramePool::GetCachedFrameCount() {
    if (m_Logger) m_Logger->EnterLog("FramePool::GetCachedFrameCount called");
    std::lock_guard<std::mutex> guard(m_Lock);
    return m_FrameVector.size();
}

std::shared_ptr<Frame> FramePool::GetFrame(FrameSpec frameSpec) {
    if (m_Logger) m_Logger->EnterLog("FramePool::GetFrame called");
    {
        std::lock_guard<std::mutex> guard(m_Lock);
        for (auto it = m_FrameVector.begin(); it != m_FrameVector.end(); ++it) {
            if ((*it)->IsIdentical(frameSpec)) {
                m_Logger->EnterLog(LogLevel::Info, "retrieving an existing cached frame from the pool");
                std::shared_ptr<Frame> p_Frame = *it;
                m_FrameVector.erase(it); // Remove the frame from the pool
                return p_Frame;
            }
        }
    }
    return AllocateFrame(frameSpec);
}

// allocation new frames upon request
std::shared_ptr<Frame> FramePool::AllocateFrame(FrameSpec frameSpec) {
    if (m_Logger) m_Logger->EnterLog("FramePool::AllocateFrame called");
    m_Logger->EnterLog(LogLevel::Info, "allocating a new frame");
    std::shared_ptr<Frame> p_Frame = std::make_shared<Frame>(frameSpec);
    {
        std::lock_guard<std::mutex> guard(m_Lock);
        m_FrameVector.push_back(p_Frame);
    }
    return p_Frame;
}

void FramePool::ReturnFrame(std::shared_ptr<Frame> p_Frame) {
    if (m_Logger) m_Logger->EnterLog("FramePool::ReturnFrame called");
    m_Logger->EnterLog(LogLevel::Info, "adding an existing frame to the pool");
    {
        std::lock_guard<std::mutex> guard(m_Lock);
        if (std::find(m_FrameVector.begin(), m_FrameVector.end(), p_Frame) == m_FrameVector.end()) {
            m_FrameVector.push_back(p_Frame);
        } else {
            m_Logger->EnterLog(LogLevel::Info, "Frame is already in the pool, skipping");
        }
    }
}