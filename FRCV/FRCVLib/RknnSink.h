#pragma once
#ifdef RKNN
#include "ISink.h"
#include <string>
#include <rknn_api.h>
#include "object_detection_utils.h"

class RknnSink : public ISink {
public:
    RknnSink(Logger* logger, const std::string& modelPath, ModelType modelType);
    ~RknnSink();

    std::string getStatus() override;
    void processFrame() override;

private:
    rknn_context context;
    std::string modelPath;
    ModelType modelType;

    void initializeModel();
    void releaseModel();
};
#endif
