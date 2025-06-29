#include "RecordSink.h"
#include <opencv2/videoio.hpp>
#include "Frame.h"


RecordSink::RecordSink(Logger* logger, string dstPath)/* : ISink(logger)*/
{
	this->dstPath = dstPath;
}

RecordSink::~RecordSink()
{
	videoWriter->release();
	delete videoWriter;
}

void RecordSink::writeFrame()
{
	//videoWriter->write(*source->getFrame());
}

string RecordSink::getVideoPath()
{
	return dstPath;
}
