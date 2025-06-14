#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/FramePool.h"
#include "../FRCVLib/Frame.h"
#include "../FRCVLib/FrameSpec.h"

class FramePoolTest : UnitTestBase
{
public:
	FramePoolTest();
	~FramePoolTest();
private:
	FramePool* framePool;
};

