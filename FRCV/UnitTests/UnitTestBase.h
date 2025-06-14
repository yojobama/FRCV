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
private:
	virtual bool innerTest();
	Logger* logger;
};

