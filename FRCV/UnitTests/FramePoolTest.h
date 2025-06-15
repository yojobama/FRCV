#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/FramePool.h"
#include "../FRCVLib/Frame.h"
#include "../FRCVLib/FrameSpec.h"

class FramePoolTest : public UnitTestBase
{
public:
	FramePoolTest();
	~FramePoolTest();
private:
	bool innerTest() override;
	FramePool* framePool;
};

