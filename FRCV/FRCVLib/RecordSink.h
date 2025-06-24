#pragma once
#include <opencv2/videoio.hpp>
#include "Logger.h"
#include <string>
#include "Frame.h"
#include "ISource.h"
#include "ISink.h"

using namespace std;

// todo: implement inheritence from ISink
class RecordSink
{
public:
	RecordSink(Logger* logger, string dstPath);
	~RecordSink();
	void writeFrame();
	string getVideoPath();
private:
	cv::VideoWriter* videoWriter;
	string dstPath;
};

