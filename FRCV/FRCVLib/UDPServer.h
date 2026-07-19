#pragma once
#include "ISink.h"
#include "ISource.h"

class UDPServer : public ISink, public ISource
{
public:
	UDPServer(std::shared_ptr<Logger> logger, std::string id, std::string dst);
private:
	std::string m_Dst;
};

