#include "ONNX_YOLO11.hpp"
#include "DrawCommands.h"

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>

// define constants
const float CONFIDENCE_THRESHOLD =0.4f;
const float IOU_THRESHOLD =0.3f;

// BoundingBox implementations
BoundingBox::BoundingBox() : x(0), y(0), width(0), height(0) {}
BoundingBox::BoundingBox(int x_, int y_, int width_, int height_) : x(x_), y(y_), width(width_), height(height_) {}

// utils implementations
namespace utils {

std::vector<std::string> getClassNames(const std::string& path) {
 std::vector<std::string> classNames;
 std::ifstream infile(path);
 if (infile) {
 std::string line;
 while (getline(infile, line)) {
 if (!line.empty() && line.back() == '\r') line.pop_back();
 classNames.emplace_back(line);
 }
 } else {
 std::cerr << "ERROR: Failed to access class name path: " << path << std::endl;
 }
 return classNames;
}

size_t vectorProduct(const std::vector<int64_t>& vector) {
 return std::accumulate(vector.begin(), vector.end(),1ull, std::multiplies<size_t>());
}

void letterBox(const cv::Mat& image, cv::Mat& outImage, const cv::Size& newShape,
 const cv::Scalar& color, bool auto_, bool scaleFill, bool scaleUp, int stride) {
 float ratio = std::min(static_cast<float>(newShape.height) / image.rows,
 static_cast<float>(newShape.width) / image.cols);
 if (!scaleUp) ratio = std::min(ratio,1.0f);
 int newUnpadW = static_cast<int>(std::round(image.cols * ratio));
 int newUnpadH = static_cast<int>(std::round(image.rows * ratio));
 int dw = newShape.width - newUnpadW;
 int dh = newShape.height - newUnpadH;
 if (auto_) { dw = (dw % stride) /2; dh = (dh % stride) /2; }
 else if (scaleFill) { newUnpadW = newShape.width; newUnpadH = newShape.height; ratio = std::min(static_cast<float>(newShape.width) / image.cols, static_cast<float>(newShape.height) / image.rows); dw =0; dh =0; }
 else {
 int padLeft = dw /2; int padRight = dw - padLeft; int padTop = dh /2; int padBottom = dh - padTop;
 if (image.cols != newUnpadW || image.rows != newUnpadH) cv::resize(image, outImage, cv::Size(newUnpadW, newUnpadH),0,0, cv::INTER_LINEAR);
 else outImage = image;
 cv::copyMakeBorder(outImage, outImage, padTop, padBottom, padLeft, padRight, cv::BORDER_CONSTANT, color);
 return;
 }
 if (image.cols != newUnpadW || image.rows != newUnpadH) cv::resize(image, outImage, cv::Size(newUnpadW, newUnpadH),0,0, cv::INTER_LINEAR);
 else outImage = image;
 int padLeft = dw /2; int padRight = dw - padLeft; int padTop = dh /2; int padBottom = dh - padTop;
 cv::copyMakeBorder(outImage, outImage, padTop, padBottom, padLeft, padRight, cv::BORDER_CONSTANT, color);
}

BoundingBox scaleCoords(const cv::Size& imageShape, BoundingBox coords, const cv::Size& imageOriginalShape, bool p_Clip) {
 BoundingBox result;
 float gain = std::min(static_cast<float>(imageShape.height) / static_cast<float>(imageOriginalShape.height), static_cast<float>(imageShape.width) / static_cast<float>(imageOriginalShape.width));
 int padX = static_cast<int>(std::round((imageShape.width - imageOriginalShape.width * gain) /2.0f));
 int padY = static_cast<int>(std::round((imageShape.height - imageOriginalShape.height * gain) /2.0f));
 result.x = static_cast<int>(std::round((coords.x - padX) / gain));
 result.y = static_cast<int>(std::round((coords.y - padY) / gain));
 result.width = static_cast<int>(std::round(coords.width / gain));
 result.height = static_cast<int>(std::round(coords.height / gain));
 if (p_Clip) {
 result.x = std::clamp(result.x,0, imageOriginalShape.width);
 result.y = std::clamp(result.y,0, imageOriginalShape.height);
 result.width = std::clamp(result.width,0, imageOriginalShape.width - result.x);
 result.height = std::clamp(result.height,0, imageOriginalShape.height - result.y);
 }
 return result;
}

void NMSBoxes(const std::vector<BoundingBox>& boundingBoxes, const std::vector<float>& scores, float scoreThreshold, float nmsThreshold, std::vector<int>& indices) {
 indices.clear();
 const size_t numBoxes = boundingBoxes.size();
 if (numBoxes ==0) return;
 std::vector<int> sortedIndices; sortedIndices.reserve(numBoxes);
 for (size_t i =0; i < numBoxes; ++i) if (scores[i] >= scoreThreshold) sortedIndices.push_back(static_cast<int>(i));
 if (sortedIndices.empty()) return;
 std::sort(sortedIndices.begin(), sortedIndices.end(), [&scores](int idx1, int idx2) { return scores[idx1] > scores[idx2]; });
 std::vector<float> areas(numBoxes,0.0f);
 for (size_t i =0; i < numBoxes; ++i) areas[i] = boundingBoxes[i].width * boundingBoxes[i].height;
 std::vector<bool> suppressed(numBoxes, false);
 for (size_t i =0; i < sortedIndices.size(); ++i) {
 int currentIdx = sortedIndices[i]; if (suppressed[currentIdx]) continue; indices.push_back(currentIdx);
 const BoundingBox& currentBox = boundingBoxes[currentIdx];
 const float x1_max = currentBox.x; const float y1_max = currentBox.y; const float x2_max = currentBox.x + currentBox.width; const float y2_max = currentBox.y + currentBox.height;
 const float area_current = areas[currentIdx];
 for (size_t j = i +1; j < sortedIndices.size(); ++j) {
 int compareIdx = sortedIndices[j]; if (suppressed[compareIdx]) continue;
 const BoundingBox& compareBox = boundingBoxes[compareIdx];
 const float x1 = std::max(x1_max, static_cast<float>(compareBox.x));
 const float y1 = std::max(y1_max, static_cast<float>(compareBox.y));
 const float x2 = std::min(x2_max, static_cast<float>(compareBox.x + compareBox.width));
 const float y2 = std::min(y2_max, static_cast<float>(compareBox.y + compareBox.height));
 const float interWidth = x2 - x1; const float interHeight = y2 - y1; if (interWidth <=0 || interHeight <=0) continue;
 const float intersection = interWidth * interHeight; const float unionArea = area_current + areas[compareIdx] - intersection; const float iou = (unionArea >0.0f) ? (intersection / unionArea) :0.0f;
 if (iou > nmsThreshold) suppressed[compareIdx] = true;
 }
 }
}

std::vector<cv::Scalar> generateColors(const std::vector<std::string>& classNames, int seed) {
 static std::unordered_map<size_t, std::vector<cv::Scalar>> colorCache;
 size_t hashKey =0; for (const auto& name : classNames) hashKey ^= std::hash<std::string>{}(name) +0x9e3779b9 + (hashKey <<6) + (hashKey >>2);
 auto it = colorCache.find(hashKey); if (it != colorCache.end()) return it->second;
 std::vector<cv::Scalar> colors; colors.reserve(classNames.size()); std::mt19937 rng(seed); std::uniform_int_distribution<int> uni(0,255);
 for (size_t i =0; i < classNames.size(); ++i) colors.emplace_back(cv::Scalar(uni(rng), uni(rng), uni(rng)));
 colorCache.emplace(hashKey, colors); return colorCache[hashKey];
}

void drawBoundingBox(cv::Mat& image, const std::vector<Detection>& detections, const std::vector<std::string>& classNames, const std::vector<cv::Scalar>& colors) {
 for (const auto& detection : detections) {
 if (detection.conf <= CONFIDENCE_THRESHOLD) continue;
 if (detection.classId <0 || static_cast<size_t>(detection.classId) >= classNames.size()) continue;
 const cv::Scalar& color = colors[detection.classId % colors.size()];
 cv::rectangle(image, cv::Point(detection.box.x, detection.box.y), cv::Point(detection.box.x + detection.box.width, detection.box.y + detection.box.height), color,2, cv::LINE_AA);
 std::string label = classNames[detection.classId] + ": " + std::to_string(static_cast<int>(detection.conf *100)) + "%";
 int fontFace = cv::FONT_HERSHEY_SIMPLEX; double fontScale = std::min(image.rows, image.cols) *0.0008; const int thickness = std::max(1, static_cast<int>(std::min(image.rows, image.cols) *0.002)); int baseline =0;
 cv::Size textSize = cv::getTextSize(label, fontFace, fontScale, thickness, &baseline);
 int labelY = std::max(detection.box.y, textSize.height +5);
 cv::Point labelTopLeft(detection.box.x, labelY - textSize.height -5);
 cv::Point labelBottomRight(detection.box.x + textSize.width +5, labelY + baseline -5);
 cv::rectangle(image, labelTopLeft, labelBottomRight, color, cv::FILLED);
 cv::putText(image, label, cv::Point(detection.box.x +2, labelY -2), fontFace, fontScale, cv::Scalar(255,255,255), thickness, cv::LINE_AA);
 }
}

void drawBoundingBoxMask(cv::Mat& image, const std::vector<Detection>& detections, const std::vector<std::string>& classNames, const std::vector<cv::Scalar>& classColors, float maskAlpha) {
 if (image.empty()) { std::cerr << "ERROR: Empty image provided to drawBoundingBoxMask." << std::endl; return; }
 const int imgHeight = image.rows; const int imgWidth = image.cols; const double fontSize = std::min(imgHeight, imgWidth) *0.0006; const int textThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) *0.001));
 cv::Mat maskImage(image.size(), image.type(), cv::Scalar::all(0)); std::vector<const Detection*> filteredDetections;
 for (const auto& detection : detections) if (detection.conf > CONFIDENCE_THRESHOLD && detection.classId >=0 && static_cast<size_t>(detection.classId) < classNames.size()) filteredDetections.emplace_back(&detection);
 for (const auto* detection : filteredDetections) { cv::Rect box(detection->box.x, detection->box.y, detection->box.width, detection->box.height); const cv::Scalar& color = classColors[detection->classId]; cv::rectangle(maskImage, box, color, cv::FILLED); }
 cv::addWeighted(maskImage, maskAlpha, image,1.0f,0, image);
 for (const auto* detection : filteredDetections) { cv::Rect box(detection->box.x, detection->box.y, detection->box.width, detection->box.height); const cv::Scalar& color = classColors[detection->classId]; cv::rectangle(image, box, color,2, cv::LINE_AA);
 std::string label = classNames[detection->classId] + ": " + std::to_string(static_cast<int>(detection->conf *100)) + "%"; int baseLine =0; cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, fontSize, textThickness, &baseLine);
 int labelY = std::max(detection->box.y, labelSize.height +5); cv::Point labelTopLeft(detection->box.x, labelY - labelSize.height -5); cv::Point labelBottomRight(detection->box.x + labelSize.width +5, labelY + baseLine -5);
 cv::rectangle(image, labelTopLeft, labelBottomRight, color, cv::FILLED); cv::putText(image, label, cv::Point(detection->box.x +2, labelY -2), cv::FONT_HERSHEY_SIMPLEX, fontSize, cv::Scalar(255,255,255), textThickness, cv::LINE_AA);
 }
}

} // namespace utils

// YOLO11Detector implementations
YOLO11Detector::YOLO11Detector(const std::string& modelPath, const std::string& labelsPath, bool useGPU)
 : env(ORT_LOGGING_LEVEL_WARNING, "ONNX_DETECTION"), sessionOptions(), session(nullptr)
{
 sessionOptions.SetIntraOpNumThreads(std::min(6, static_cast<int>(std::thread::hardware_concurrency())));
 sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
 std::vector<std::string> availableProviders = Ort::GetAvailableProviders();
 auto cudaAvailable = std::find(availableProviders.begin(), availableProviders.end(), "CUDAExecutionProvider");
 OrtCUDAProviderOptions cudaOption;
 if (useGPU && cudaAvailable != availableProviders.end()) {
 std::cout << "Inference device: GPU" << std::endl;
 sessionOptions.AppendExecutionProvider_CUDA(cudaOption);
 } else {
 if (useGPU) std::cout << "GPU is not supported by your ONNXRuntime build. Fallback to CPU." << std::endl;
 std::cout << "Inference device: CPU" << std::endl;
 }
#ifdef _WIN32
 std::wstring w_modelPath(modelPath.begin(), modelPath.end());
 session = Ort::Session(env, w_modelPath.c_str(), sessionOptions);
#else
 session = Ort::Session(env, modelPath.c_str(), sessionOptions);
#endif
 Ort::AllocatorWithDefaultOptions allocator;
 Ort::TypeInfo inputTypeInfo = session.GetInputTypeInfo(0);
 std::vector<int64_t> inputTensorShapeVec = inputTypeInfo.GetTensorTypeAndShapeInfo().GetShape();
 isDynamicInputShape = (inputTensorShapeVec.size() >=4) && (inputTensorShapeVec[2] == -1 && inputTensorShapeVec[3] == -1);
 auto input_name = session.GetInputNameAllocated(0, allocator);
 inputNodeNameAllocatedStrings.push_back(std::move(input_name));
 inputNames.push_back(inputNodeNameAllocatedStrings.back().get());
 auto output_name = session.GetOutputNameAllocated(0, allocator);
 outputNodeNameAllocatedStrings.push_back(std::move(output_name));
 outputNames.push_back(outputNodeNameAllocatedStrings.back().get());
 if (inputTensorShapeVec.size() >=4) inputImageShape = cv::Size(static_cast<int>(inputTensorShapeVec[3]), static_cast<int>(inputTensorShapeVec[2]));
 else throw std::runtime_error("Invalid input tensor shape.");
 numInputNodes = session.GetInputCount();
 numOutputNodes = session.GetOutputCount();
 classNames = utils::getClassNames(labelsPath);
 classColors = utils::generateColors(classNames);
 std::cout << "Model loaded successfully with " << numInputNodes << " input nodes and " << numOutputNodes << " output nodes." << std::endl;
}

cv::Mat YOLO11Detector::preprocess(const cv::Mat& image, float*& blob, std::vector<int64_t>& inputTensorShape) {
 cv::Mat resizedImage;
 utils::letterBox(image, resizedImage, inputImageShape, cv::Scalar(114,114,114), isDynamicInputShape, false, true,32);
 cv::Mat rgbImage;
 cv::cvtColor(resizedImage, rgbImage, cv::COLOR_BGR2RGB);
 rgbImage.convertTo(rgbImage, CV_32FC3,1.0f /255.0f);
 blob = new float[rgbImage.cols * rgbImage.rows * rgbImage.channels()];
 std::vector<cv::Mat> chw(rgbImage.channels());
 for (int i =0; i < rgbImage.channels(); ++i) chw[i] = cv::Mat(rgbImage.rows, rgbImage.cols, CV_32FC1, blob + i * rgbImage.cols * rgbImage.rows);
 cv::split(rgbImage, chw);
 return rgbImage;
}

std::vector<Detection> YOLO11Detector::postprocess(const cv::Size& originalImageSize, const cv::Size& resizedImageShape, const std::vector<Ort::Value>& outputTensors, float confThreshold, float iouThreshold) {
 std::vector<Detection> detections;
 const float* rawOutput = outputTensors[0].GetTensorData<float>();
 const std::vector<int64_t> outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
 const size_t num_features = outputShape[1];
 const size_t num_detections = outputShape[2];
 if (num_detections ==0) return detections;
 const int numClasses = static_cast<int>(num_features) -4;
 if (numClasses <=0) return detections;
 std::vector<BoundingBox> boxes; boxes.reserve(num_detections);
 std::vector<float> confs; confs.reserve(num_detections);
 std::vector<int> classIds; classIds.reserve(num_detections);
 std::vector<BoundingBox> nms_boxes; nms_boxes.reserve(num_detections);
 const float* ptr = rawOutput;
 for (size_t d =0; d < num_detections; ++d) {
 float centerX = ptr[0 * num_detections + d];
 float centerY = ptr[1 * num_detections + d];
 float width = ptr[2 * num_detections + d];
 float height = ptr[3 * num_detections + d];
 int classId = -1; float maxScore = -FLT_MAX;
 for (int c =0; c < numClasses; ++c) { const float score = ptr[d + (4 + c) * num_detections]; if (score > maxScore) { maxScore = score; classId = c; } }
 if (maxScore > confThreshold) {
 float left = centerX - width /2.0f; float top = centerY - height /2.0f;
 BoundingBox scaledBox = utils::scaleCoords(resizedImageShape, BoundingBox(left, top, width, height), originalImageSize, true);
 BoundingBox roundedBox; roundedBox.x = std::round(scaledBox.x); roundedBox.y = std::round(scaledBox.y); roundedBox.width = std::round(scaledBox.width); roundedBox.height = std::round(scaledBox.height);
 BoundingBox nmsBox = roundedBox; nmsBox.x += classId *7680; nmsBox.y += classId *7680;
 nms_boxes.emplace_back(nmsBox); boxes.emplace_back(roundedBox); confs.emplace_back(maxScore); classIds.emplace_back(classId);
 }
 }
 std::vector<int> indices; utils::NMSBoxes(nms_boxes, confs, confThreshold, iouThreshold, indices);
 detections.reserve(indices.size()); for (const int idx : indices) detections.emplace_back(Detection{ boxes[idx], confs[idx], classIds[idx] });
 return detections;
}

std::vector<Detection> YOLO11Detector::detect(const cv::Mat& image, float confThreshold, float iouThreshold) {
 if (image.empty()) { std::cerr << "Error: Empty image provided to detector" << std::endl; return {}; }
 float* blobPtr = nullptr; std::vector<int64_t> inputTensorShape = {1,3, inputImageShape.height, inputImageShape.width };
 cv::Mat preprocessedImage = preprocess(image, blobPtr, inputTensorShape);
 size_t inputTensorSize = utils::vectorProduct(inputTensorShape);
 std::vector<float> inputTensorValues(blobPtr, blobPtr + inputTensorSize);
 delete[] blobPtr;
 static Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
 Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo, inputTensorValues.data(), inputTensorSize, inputTensorShape.data(), inputTensorShape.size());
 std::vector<Ort::Value> outputTensors = session.Run(Ort::RunOptions{ nullptr }, inputNames.data(), &inputTensor, numInputNodes, outputNames.data(), numOutputNodes);
 cv::Size resizedImageShape(static_cast<int>(inputTensorShape[3]), static_cast<int>(inputTensorShape[2]));
 std::vector<Detection> detections = postprocess(image.size(), resizedImageShape, outputTensors, confThreshold, iouThreshold);
 return detections;
}
