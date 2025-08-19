#include "RecordSink.h"
#include <opencv2/videoio.hpp>
#include "Frame.h"

RecordSink::RecordSink(Logger* logger, string dstPath) : ISink(logger)
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

void RecordSink::ProcessFrame()
{
	videoWriter->write(*m_Source->GetLatestFrame());
}

void RecordSink::CreatePreview()
{
	m_PreviewFrame = m_Source->GetLatestFrame();
}
