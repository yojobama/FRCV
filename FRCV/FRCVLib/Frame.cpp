#include "Frame.h"

Frame::Frame(FrameSpec spec)
    : cv::Mat(spec.getHeight(), spec.getWidth(), spec.getType()), spec(spec) {
}

bool Frame::isIdentical(FrameSpec spec) {
    FrameSpec* cmp = &this->spec;
    return 
        spec.getHeight() == cmp->getHeight() &&
        spec.getWidth() == cmp->getWidth() &&
        spec.getType() == cmp->getType();
}