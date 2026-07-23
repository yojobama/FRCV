#include "RecordSink.h"
#include <opencv2/videoio.hpp>

RecordSink::RecordSink(std::shared_ptr<Logger> logger, string id, string dstPath) : ISink(logger, 1, false, true, id)
{
	this->dstPath = dstPath;
}

RecordSink::~RecordSink()
{
	videoWriter->release();
	delete videoWriter;
}

string RecordSink::getVideoPath()
{
	return dstPath;
}

void RecordSink::Process(std::vector<SourceResult> sources)
{
	videoWriter->write(sources[0].frame.value());
}
//
//void RecordSink::CreatePreview()
//{
//	m_PreviewFrame = m_Source->GetLatestFrame();
//}
