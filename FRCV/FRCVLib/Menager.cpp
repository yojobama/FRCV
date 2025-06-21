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
	string returnString;
	returnString += "{[";

	auto iterator = sinks.begin();

	while (iterator != sinks.end()) {
		returnString += "\"";
		returnString += iterator->second.getStatus();
		returnString += "\",";
		iterator++;
	}

	returnString += "]}";

	return returnString;
}

string Manager::getSinkStatusById(int sinkId)
{
	ISink* sink;

	if (sinks.find(sinkId) != sinks.end()) {
		return "";
	}
	sink = &sinks.find(sinkId)->second;

	return sink->getStatus();
}

string Manager::getSinkResult(int sinkId)
{
	if (results.find(sinkId) == results.end()) {
		return "";
	}
	return results.find(sinkId)->second;
}

string Manager::getAllSinkResults()
{
	string returnString;
	returnString += "{[";

	auto iterator = results.begin();

	while (iterator != results.end()) {
		returnString += "{";
		returnString += iterator->second;
		returnString += "},";
		iterator++;
	}

	returnString += "]}";

	return returnString;
}

bool Manager::setSinkResult(int sinkId, string result)
{
	if (results.find(sinkId) == results.end()) {
		return false;
	}
	else {
		results.find(sinkId)->second = result;
		return true;
	}
}

int Manager::generateUUID()
{
	std::mt19937 engine(std::chrono::high_resolution_clock::now().time_since_epoch().count());

	std::uniform_int_distribution<int> dist(-2147483648, 2147483647);

	int randomNumber = dist(engine);
	return 0;
}
