#include "UnitTestBase.h"

UnitTestBase::UnitTestBase()
{
	this->logger = new Logger();
}

UnitTestBase::~UnitTestBase()
{
	delete logger;
}

void UnitTestBase::doTest()
{
	if (!this->innerTest()) { // Call the innerTest function
		std::vector<Log*>* logs = logger->GetAllLogs();
		for (int i = 0; i < logs->size(); i++) {
			std::cout << logs->at(i)->GetMessage() << std::endl;
		}
	}
}
