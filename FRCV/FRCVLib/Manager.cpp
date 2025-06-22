// ...existing includes...
#include "Menager.h"
// ...existing code...

Manager::Manager() {
    // ...existing code...
    if (logger) logger->enterLog("Manager constructed");
}

Manager::~Manager() {
    if (logger) logger->enterLog("Manager destructed");
    // ...existing code...
}

vector<int> Manager::getAllSinks() {
    if (logger) logger->enterLog("Manager::getAllSinks called");
    // ...existing code...
}

vector<CameraHardwareInfo> Manager::enumerateAvailableCameras() {
    if (logger) logger->enterLog("Manager::enumerateAvailableCameras called");
    // ...existing code...
}

bool Manager::bindSourceToSink(int sourceId, int sinkId) {
    if (logger) logger->enterLog("Manager::bindSourceToSink called");
    // ...existing code...
}

bool Manager::unbindSourceFromSink(int sinkId) {
    if (logger) logger->enterLog("Manager::unbindSourceFromSink called");
    // ...existing code...
}

int Manager::createCameraSource(CameraHardwareInfo info) {
    if (logger) logger->enterLog("Manager::createCameraSource called");
    // ...existing code...
}

int Manager::createVideoFileSource(string path) {
    if (logger) logger->enterLog("Manager::createVideoFileSource called");
    // ...existing code...
}

int Manager::createImageFileSource(string path) {
    if (logger) logger->enterLog("Manager::createImageFileSource called");
    // ...existing code...
}

int Manager::createApriltagSink() {
    if (logger) logger->enterLog("Manager::createApriltagSink called");
    // ...existing code...
}

int Manager::createObjectDetectionSink() {
    if (logger) logger->enterLog("Manager::createObjectDetectionSink called");
    // ...existing code...
}

bool Manager::startAllSinks() {
    if (logger) logger->enterLog("Manager::startAllSinks called");
    // ...existing code...
}

bool Manager::stopAllSinks() {
    if (logger) logger->enterLog("Manager::stopAllSinks called");
    // ...existing code...
}

string Manager::getAllSinkStatus() {
    if (logger) logger->enterLog("Manager::getAllSinkStatus called");
    // ...existing code...
}

string Manager::getSinkStatusById(int sinkId) {
    if (logger) logger->enterLog("Manager::getSinkStatusById called");
    // ...existing code...
}

string Manager::getSinkResult(int sinkId) {
    if (logger) logger->enterLog("Manager::getSinkResult called");
    // ...existing code...
}

string Manager::getAllSinkResults() {
    if (logger) logger->enterLog("Manager::getAllSinkResults called");
    // ...existing code...
}

vector<Log> Manager::getAllLogs() {
    if (logger) logger->enterLog("Manager::getAllLogs called");
    // ...existing code...
}

void Manager::clearAllLogs() {
    if (logger) logger->enterLog("Manager::clearAllLogs called");
    // ...existing code...
}

bool Manager::setSinkResult(int sinkId, string result) {
    if (logger) logger->enterLog("Manager::setSinkResult called");
    // ...existing code...
}

int Manager::generateUUID() {
    if (logger) logger->enterLog("Manager::generateUUID called");
    // ...existing code...
}

// ...rest of file...