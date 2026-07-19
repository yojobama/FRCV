#pragma once
#include "../FRCVLib/Logger.h"
#include <vector>
#include <string>
#include <iostream>

class UnitTestBase
{
public:
	UnitTestBase();
	~UnitTestBase();
	void doTest();
protected:
	std::shared_ptr<Logger> logger;
private:
	virtual bool innerTest() = 0;
};

