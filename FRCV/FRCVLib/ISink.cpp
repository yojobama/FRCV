#include "ISink.h"

ISink::ISink(Logger* logger) {
	this->logger = logger;
}

bool ISink::bindSource(ISource* source) {
	if (source == nullptr) {
		logger->enterLog(ERROR, "ISink::bindSource: Source is null");
		return false;
	}
	this->source = source;
	return true;
}

bool ISink::unbindSource() {
	if (source == nullptr) {
		logger->enterLog(ERROR, "ISink::unbindSource: Source is already null");
		return false;
	}
	source = nullptr;
	return true;
}