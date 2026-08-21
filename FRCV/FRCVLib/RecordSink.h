#pragma once
#include "Logger.h"
#include <string>
#include "ISink.h"

using namespace std;

namespace cv {
	class VideoWriter;
};

// todo: implement inheritence from ISink
class RecordSink : public ISink
{
public:
	RecordSink(std::shared_ptr<Logger> logger, std::string id, string dstPath);
	~RecordSink();
	string getVideoPath();

private:
	void Process(std::vector<SourceResult> sources) override;

	cv::VideoWriter* videoWriter;
	string dstPath;
};

