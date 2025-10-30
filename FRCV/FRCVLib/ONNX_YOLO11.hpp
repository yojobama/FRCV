#pragma once

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <random>
#include <unordered_map>
#include <thread>

// Constants
extern const float CONFIDENCE_THRESHOLD;
extern const float IOU_THRESHOLD;

struct BoundingBox {
 int x;
 int y;
 int width;
 int height;

 BoundingBox();
 BoundingBox(int x_, int y_, int width_, int height_);
};

struct Detection {
 BoundingBox box;
 float conf;
 int classId;
};

namespace utils {
// Utility declarations (defined in .cpp to avoid multiple definitions in headers)
std::vector<std::string> getClassNames(const std::string& path);
size_t vectorProduct(const std::vector<int64_t>& vector);
void letterBox(const cv::Mat& image, cv::Mat& outImage, const cv::Size& newShape,
 const cv::Scalar& color = cv::Scalar(114,114,114), bool auto_ = true,
 bool scaleFill = false, bool scaleUp = true, int stride =32);
BoundingBox scaleCoords(const cv::Size& imageShape, BoundingBox coords,
 const cv::Size& imageOriginalShape, bool p_Clip);
void NMSBoxes(const std::vector<BoundingBox>& boundingBoxes,
 const std::vector<float>& scores,
 float scoreThreshold,
 float nmsThreshold,
 std::vector<int>& indices);
std::vector<cv::Scalar> generateColors(const std::vector<std::string>& classNames, int seed =42);
void drawBoundingBox(cv::Mat& image, const std::vector<Detection>& detections,
 const std::vector<std::string>& classNames, const std::vector<cv::Scalar>& colors);
void drawBoundingBoxMask(cv::Mat& image, const std::vector<Detection>& detections,
 const std::vector<std::string>& classNames, const std::vector<cv::Scalar>& classColors,
 float maskAlpha =0.4f);

} // namespace utils

class YOLO11Detector {
public:
 YOLO11Detector(const std::string& modelPath, const std::string& labelsPath, bool useGPU = false);
 std::vector<Detection> detect(const cv::Mat& image, float confThreshold =0.4f, float iouThreshold =0.45f);

 void drawBoundingBox(cv::Mat& image, const std::vector<Detection>& detections) const;
 void drawBoundingBoxMask(cv::Mat& image, const std::vector<Detection>& detections, float maskAlpha =0.4f) const;

private:
 Ort::Env env;
 Ort::SessionOptions sessionOptions;
 Ort::Session session;
 bool isDynamicInputShape;
 cv::Size inputImageShape;

 std::vector<Ort::AllocatedStringPtr> inputNodeNameAllocatedStrings;
 std::vector<const char*> inputNames;
 std::vector<Ort::AllocatedStringPtr> outputNodeNameAllocatedStrings;
 std::vector<const char*> outputNames;

 size_t numInputNodes;
 size_t numOutputNodes;

 std::vector<std::string> classNames;
 std::vector<cv::Scalar> classColors;

 cv::Mat preprocess(const cv::Mat& image, float*& blob, std::vector<int64_t>& inputTensorShape);
 std::vector<Detection> postprocess(const cv::Size& originalImageSize, const cv::Size& resizedImageShape,
 const std::vector<Ort::Value>& outputTensors,
 float confThreshold, float iouThreshold);
};