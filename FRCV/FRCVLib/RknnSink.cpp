//#include "RknnSink.h"
//#include "Frame.h" // Ensure the Frame class is fully defined
//#include <iostream>
//
//RknnSink::RknnSink(Logger* logger, const std::string& modelPath, ModelType modelType)
//    : ISink(logger), modelPath(modelPath), modelType(modelType), context(0) {
//    if (logger) logger->enterLog("RknnSink constructed");
//    initializeModel();
//}
//
//RknnSink::~RknnSink() {
//    releaseModel();
//    if (logger) logger->enterLog("RknnSink destructed");
//}
//
//void RknnSink::initializeModel() {
//    if (logger) logger->enterLog("Initializing RKNN model");
//    int ret = rknn_init(&context, const_cast<char*>(modelPath.c_str()), 0, 0, nullptr); // Fix const void* to void*
//    if (ret != RKNN_SUCC) {
//        if (logger) logger->enterLog(ERROR, "Failed to initialize RKNN model");
//        throw std::runtime_error("Failed to initialize RKNN model");
//    }
//
//    // Query model input/output information
//    rknn_input_output_num io_num;
//    ret = rknn_query(context, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
//    if (ret != RKNN_SUCC) {
//        if (logger) logger->enterLog(ERROR, "Failed to query RKNN model input/output information");
//        throw std::runtime_error("Failed to query RKNN model input/output information");
//    }
//
//    if (logger) logger->enterLog("RKNN model initialized successfully");
//}
//
//void RknnSink::releaseModel() {
//    if (context) {
//        rknn_destroy(context);
//        context = 0;
//    }
//}
//
//std::string RknnSink::getStatus() {
//    return context ? "Model loaded" : "Model not loaded";
//}
//
//void RknnSink::processFrame() {
//    if (!source) {
//        if (logger) logger->enterLog(ERROR, "No source bound to RknnSink");
//        return;
//    }
//
//    auto frame = source->getLatestFrame();
//    if (!frame) {
//        if (logger) logger->enterLog(ERROR, "Failed to get frame from source");
//        return;
//    }
//
//    // Prepare input tensor
//    rknn_input input = {0};
//    input.index = 0;
//    input.buf = frame->data; // Ensure Frame is fully defined
//    input.size = frame->total() * frame->elemSize();
//    input.pass_through = false;
//    input.type = RKNN_TENSOR_UINT8;
//    input.fmt = RKNN_TENSOR_NHWC;
//
//    int ret = rknn_inputs_set(context, 1, &input);
//    if (ret != RKNN_SUCC) {
//        if (logger) logger->enterLog(ERROR, "Failed to set RKNN input");
//        return;
//    }
//
//    // Run inference
//    ret = rknn_run(context, nullptr);
//    if (ret != RKNN_SUCC) {
//        if (logger) logger->enterLog(ERROR, "Failed to run RKNN inference");
//        return;
//    }
//
//    // Retrieve output
//    rknn_output output = {0};
//    output.want_float = true;
//    ret = rknn_outputs_get(context, 1, &output, nullptr);
//    if (ret != RKNN_SUCC) {
//        if (logger) logger->enterLog(ERROR, "Failed to get RKNN output");
//        return;
//    }
//
//    // Process output for YOLOv8/YOLOv11
//    if (modelType == YOLOV8) {
//        if (logger) logger->enterLog("Processing output for YOLOv8");
//        // Add YOLOv8-specific output processing logic here
//    } else if (modelType == YOLOV11) {
//        if (logger) logger->enterLog("Processing output for YOLOv11");
//        // Add YOLOv11-specific output processing logic here
//    }
//
//    // Release output
//    rknn_outputs_release(context, 1, &output);
//}
