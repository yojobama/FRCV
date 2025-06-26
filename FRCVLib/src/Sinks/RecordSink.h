#pragma once
#include "../Logger.h"
#include <string>

class Frame;

using namespace std;

namespace cv {
	class VideoWriter;
};

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

