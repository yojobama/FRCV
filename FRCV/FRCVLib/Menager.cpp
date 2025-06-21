#include "Menager.h"

Manager::Manager()
{
	logger = new Logger();
}

vector<int> Manager::getAllSinks()
{
	vector<int> returnVector;

	auto iterator = sinks.begin();

	while (iterator != sinks.end())
	{
		returnVector.push_back(iterator->first);
		iterator++;
	}
	
	return returnVector;
}

bool Manager::bindSourceToSink(int sourceId, int sinkId) {
	ISource* source;
	ISink* sink;

	if (sources.find(sourceId) == sources.end()) {
		return false;
	}
	source = &sources.find(sourceId)->second;

	if (sinks.find(sinkId) != sinks.end()) {
		return false;
	}
	sink = &sinks.find(sinkId)->second;

	return sink->bindSource(source);
}

bool Manager::unbindSourceFromSink(int sinkId) {
	ISink* sink;

	if (sinks.find(sinkId) != sinks.end()) {
		return false;
	}
	sink = &sinks.find(sinkId)->second;

	return sink->unbindSource();
}

int Manager::createCameraSource(CameraHardwareInfo info)
{
	return 0;
}

int Manager::createVideoFileSource(string path)
{
	int id = generateUUID();

	VideoFileFrameSource source = VideoFileFrameSource(logger, path, &framePool);

	sources.emplace(id, source);

	return id;
}

int Manager::createImageFileSource(string path)
{
	int id = generateUUID();

	ImageFileFrameSource source = ImageFileFrameSource(path, logger);

	sources.emplace(id, source);

	return id;
}

int Manager::createApriltagSink()
{
	int id = generateUUID();

	ApriltagSink sink = ApriltagSink(logger);

	sinks.emplace(id, sink);

	return id;
}

int Manager::createObjectDetectionSink()
{
	return 0;
}

bool Manager::startAllSinks()
{
	return false;
}

bool Manager::stopAllSinks()
{
	return false;
}

string Manager::getAllSinkStatus()
{
	return string();
}

string Manager::getSinkStatusById(int sinkId)
{
	return string();
}

string Manager::getSinkResult(int sinkId)
{
	return string();
}

string Manager::getAllSinkResults()
{
	return string();
}

void Manager::setSinkResult(int sinkId, string result)
{
}
