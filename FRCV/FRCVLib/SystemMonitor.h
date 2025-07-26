#pragma once
#include <pthread.h>
#include <string>


class SystemMonitor
{
public:
	SystemMonitor();
	~SystemMonitor();
private:
	pthread_t monitorThread; 
	void* monitorLoop(void* arg);
};

