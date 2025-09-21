#include "Frame.h"
#include "FramePool.h"

Frame::Frame(FrameSpec spec)
    : cv::Mat(spec.GetHeight(), spec.GetWidth(), spec.GetType()), m_Spec(spec) {
}

bool Frame::IsIdentical(FrameSpec spec) {
    FrameSpec* p_Cmp = &this->m_Spec;
    return 
        spec.GetHeight() == p_Cmp->GetHeight() &&
        spec.GetWidth() == p_Cmp->GetWidth() &&
        spec.GetType() == p_Cmp->GetType();
}

FrameSpec Frame::GetSpec()
{
    return FrameSpec(cols, rows, type());
}

uint64_t Frame::GetFrameNumber()
{
    return m_FrameNumber;
}

void Frame::SetFrameNumber(uint64_t frameNumber)
{
    m_FrameNumber = frameNumber;
}
