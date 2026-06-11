#pragma once

#include <string>
#include <memory>

#include <opencv2/opencv.hpp>

#include "ISink.h"

class VideoFileRecorder : ISink
{
public:
	VideoFileRecorder(std::shared_ptr<Logger> logger, const std::string& dst, const std::string& id, int height, int width, int fps);
private:
	void Process(std::vector<SourceResult> sources) override;

	cv::VideoWriter m_VideoWriter;
	std::string m_Dst;
	std::shared_ptr<ISource> m_Source;
};

