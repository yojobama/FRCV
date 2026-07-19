#include "VideoFileRecorder.h"

VideoFileRecorder::VideoFileRecorder(std::shared_ptr<Logger> logger, const std::string& dst, const std::string& id, int height, int width, int fps) : ISink(logger, 1, false, true, id)
{
	m_Dst = dst;
	m_VideoWriter = cv::VideoWriter(dst, cv::VideoWriter::fourcc('x', '2', '6', '4'), fps, cv::Size(width, height)); // Todo: check if the fourcc code is correct
}

void VideoFileRecorder::Process(std::vector<SourceResult> sources)
{
	const SourceResult& source = sources.front();
	if (source.frame.has_value()) {
		m_VideoWriter << source.frame.value();
	}
}
