#include "IFrameSource.h"

IFrameSource::IFrameSource(FramePool* framePool, Logger* logger)
	: frameSpec(0, 0, 0) // Initialize frameSpec with default values
{
	this->framePool = framePool;
	this->logger = logger;
}
