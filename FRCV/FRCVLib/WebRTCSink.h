#pragma once
#include <rtc/rtc.hpp>
#include "ISink.h"

#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

const int BUFFER_SIZE = 0;

class WebRTCSink : public ISink
{
public:
	WebRTCSink(Logger* logger);
private:

};

