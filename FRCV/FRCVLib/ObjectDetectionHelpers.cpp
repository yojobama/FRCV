#include "ObjectDetectionHelpers.h"
#include <fstream>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include <random>
#include <cfloat>
#include <cmath>
#include <iostream>

namespace {
    // Local confidence threshold to match behavior from ONNX_YOLO11.hpp utils
    constexpr float LOCAL_CONFIDENCE_THRESHOLD = 0.4f;
}

std::vector<std::string> ObjectDetectionHelpers::GetClassNames(const std::string& labelsPath)
{
    std::vector<std::string> classNames;
    std::ifstream infile(labelsPath);
    if (infile) {
        std::string line;
        while (std::getline(infile, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            classNames.emplace_back(line);
        }
    }
    else {
        std::cerr << "ERROR: Failed to access class name path: " << labelsPath << std::endl;
    }
    return classNames;
}

unsigned long ObjectDetectionHelpers::VectorProduct(const std::vector<long>& vector)
{
    return std::accumulate(vector.begin(), vector.end(), 1UL, [](unsigned long a, long b) {
        return a * static_cast<unsigned long>(b);
    });
}

void ObjectDetectionHelpers::LetterBox(cv::Mat& image, cv::Mat& output, cv::Size newShape, cv::Scalar colour, bool auto_, bool scaleFiil, bool scaleUp, int stride)
{
    // Adapted from utils::letterBox implementation
    if (image.empty()) {
        output = image;
        return;
    }

    float r = std::min(static_cast<float>(newShape.height) / image.rows,
                       static_cast<float>(newShape.width)  / image.cols);

    if (!scaleUp) {
        r = std::min(r, 1.0f);
    }

    int newUnpadW = static_cast<int>(std::round(image.cols * r));
    int newUnpadH = static_cast<int>(std::round(image.rows * r));

    int dw = newShape.width - newUnpadW;
    int dh = newShape.height - newUnpadH;

    if (auto_) {
        dw = (dw % stride) / 2;
        dh = (dh % stride) / 2;
    }
    else if (scaleFiil) {
        newUnpadW = newShape.width;
        newUnpadH = newShape.height;
        r = std::min(static_cast<float>(newShape.width) / image.cols,
                     static_cast<float>(newShape.height) / image.rows);
        dw = 0;
        dh = 0;
    }
    else {
        int padLeft = dw / 2;
        int padRight = dw - padLeft;
        int padTop = dh / 2;
        int padBottom = dh - padTop;

        if (image.cols != newUnpadW || image.rows != newUnpadH) {
            cv::resize(image, output, cv::Size(newUnpadW, newUnpadH), 0, 0, cv::INTER_LINEAR);
        }
        else {
            output = image;
        }

        cv::copyMakeBorder(output, output, padTop, padBottom, padLeft, padRight, cv::BORDER_CONSTANT, colour);
        return;
    }

    if (image.cols != newUnpadW || image.rows != newUnpadH) {
        cv::resize(image, output, cv::Size(newUnpadW, newUnpadH), 0, 0, cv::INTER_LINEAR);
    }
    else {
        output = image;
    }

    int padLeft = dw / 2;
    int padRight = dw - padLeft;
    int padTop = dh / 2;
    int padBottom = dh - padTop;

    cv::copyMakeBorder(output, output, padTop, padBottom, padLeft, padRight, cv::BORDER_CONSTANT, colour);
}

BoundingBox ObjectDetectionHelpers::ScaleBox(const cv::Size imageShape, BoundingBox coords, cv::Size& imgOriginalShape, bool p_Clip)
{
    BoundingBox result;
    float gain = std::min(static_cast<float>(imageShape.height) / static_cast<float>(imgOriginalShape.height),
                          static_cast<float>(imageShape.width)  / static_cast<float>(imgOriginalShape.width));

    int padX = static_cast<int>(std::round((imageShape.width  - imgOriginalShape.width  * gain) / 2.0f));
    int padY = static_cast<int>(std::round((imageShape.height - imgOriginalShape.height * gain) / 2.0f));

    result.x = static_cast<int>(std::round((coords.x - padX) / gain));
    result.y = static_cast<int>(std::round((coords.y - padY) / gain));
    result.width  = static_cast<int>(std::round(coords.width  / gain));
    result.height = static_cast<int>(std::round(coords.height / gain));

    if (p_Clip) {
        if (result.x < 0) result.x = 0;
        if (result.y < 0) result.y = 0;
        if (result.x > imgOriginalShape.width) result.x = imgOriginalShape.width;
        if (result.y > imgOriginalShape.height) result.y = imgOriginalShape.height;
        if (result.width < 0) result.width = 0;
        if (result.height < 0) result.height = 0;
        if (result.x + result.width > imgOriginalShape.width) result.width = imgOriginalShape.width - result.x;
        if (result.y + result.height > imgOriginalShape.height) result.height = imgOriginalShape.height - result.y;
    }

    return result;
}

void ObjectDetectionHelpers::NMSBoxes(std::vector<BoundingBox>& boundingBoxes, std::vector<float> scores, float scoreThreashold, float nmsThreashold, std::vector<int>& indices)
{
    indices.clear();
    const size_t numBoxes = boundingBoxes.size();
    if (numBoxes == 0) {
        return;
    }

    // Filter by score threshold
    std::vector<int> sortedIndices;
    sortedIndices.reserve(numBoxes);
    for (size_t i = 0; i < numBoxes; ++i) {
        if (scores[i] >= scoreThreashold) {
            sortedIndices.push_back(static_cast<int>(i));
        }
    }

    if (sortedIndices.empty())
        return;

    std::sort(sortedIndices.begin(), sortedIndices.end(), [&scores](int a, int b) {
        return scores[a] > scores[b];
    });

    std::vector<float> areas(numBoxes, 0.0f);
    for (size_t i = 0; i < numBoxes; ++i) {
        areas[i] = static_cast<float>(boundingBoxes[i].width) * static_cast<float>(boundingBoxes[i].height);
    }

    std::vector<bool> suppressed(numBoxes, false);

    for (size_t i = 0; i < sortedIndices.size(); ++i) {
        int currentIdx = sortedIndices[i];
        if (suppressed[currentIdx])
            continue;

        indices.push_back(currentIdx);

        const BoundingBox& currentBox = boundingBoxes[currentIdx];
        const float x1_max = static_cast<float>(currentBox.x);
        const float y1_max = static_cast<float>(currentBox.y);
        const float x2_max = static_cast<float>(currentBox.x + currentBox.width);
        const float y2_max = static_cast<float>(currentBox.y + currentBox.height);
        const float area_current = areas[currentIdx];

        for (size_t j = i + 1; j < sortedIndices.size(); ++j) {
            int compareIdx = sortedIndices[j];
            if (suppressed[compareIdx])
                continue;

            const BoundingBox& compareBox = boundingBoxes[compareIdx];
            const float x1 = std::max(x1_max, static_cast<float>(compareBox.x));
            const float y1 = std::max(y1_max, static_cast<float>(compareBox.y));
            const float x2 = std::min(x2_max, static_cast<float>(compareBox.x + compareBox.width));
            const float y2 = std::min(y2_max, static_cast<float>(compareBox.y + compareBox.height));

            const float interWidth = x2 - x1;
            const float interHeight = y2 - y1;

            if (interWidth <= 0.0f || interHeight <= 0.0f)
                continue;

            const float intersection = interWidth * interHeight;
            const float unionArea = area_current + areas[compareIdx] - intersection;
            const float iou = (unionArea > 0.0f) ? (intersection / unionArea) : 0.0f;

            if (iou > nmsThreashold) {
                suppressed[compareIdx] = true;
            }
        }
    }
}

std::vector<cv::Scalar> ObjectDetectionHelpers::GenerateColours(std::vector<std::string> classNames, int seed)
{
    static std::unordered_map<size_t, std::vector<cv::Scalar>> colorCache;
    size_t hashKey = 0;
    for (const auto& name : classNames) {
        hashKey ^= std::hash<std::string>{}(name) + 0x9e3779b9 + (hashKey << 6) + (hashKey >> 2);
    }

    auto it = colorCache.find(hashKey);
    if (it != colorCache.end()) {
        return it->second;
    }

    std::vector<cv::Scalar> colors;
    colors.reserve(classNames.size());
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> uni(0, 255);

    for (size_t i = 0; i < classNames.size(); ++i) {
        colors.emplace_back(cv::Scalar(uni(rng), uni(rng), uni(rng)));
    }

    colorCache.emplace(hashKey, colors);
    return colorCache[hashKey];
}

void ObjectDetectionHelpers::DrawBoundingBoxes(cv::Mat& image, std::vector<Detection> detections, std::vector<std::string> classNames, std::vector<cv::Scalar> colours)
{
    if (image.empty()) return;
    const int fontFace = cv::FONT_HERSHEY_SIMPLEX;

    for (const auto& detection : detections) {
        if (detection.conf <= LOCAL_CONFIDENCE_THRESHOLD)
            continue;

        if (detection.classId < 0 || static_cast<size_t>(detection.classId) >= classNames.size())
            continue;

        const cv::Scalar& color = colours.empty() ? cv::Scalar(0, 255, 0) : colours[detection.classId % colours.size()];

        cv::rectangle(image,
            cv::Point(detection.box.x, detection.box.y),
            cv::Point(detection.box.x + detection.box.width, detection.box.y + detection.box.height),
            color, 2, cv::LINE_AA);

        std::string label = classNames[detection.classId] + ": " + std::to_string(static_cast<int>(detection.conf * 100)) + "%";

        double fontScale = std::min(image.rows, image.cols) * 0.0008;
        const int thickness = std::max(1, static_cast<int>(std::min(image.rows, image.cols) * 0.002));
        int baseline = 0;

        cv::Size textSize = cv::getTextSize(label, fontFace, fontScale, thickness, &baseline);

        int labelY = std::max(detection.box.y, textSize.height + 5);
        cv::Point labelTopLeft(detection.box.x, labelY - textSize.height - 5);
        cv::Point labelBottomRight(detection.box.x + textSize.width + 5, labelY + baseline - 5);

        cv::rectangle(image, labelTopLeft, labelBottomRight, color, cv::FILLED);
        cv::putText(image, label, cv::Point(detection.box.x + 2, labelY - 2), fontFace, fontScale, cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);
    }
}

void ObjectDetectionHelpers::DrawBoundingBoxMask(cv::Mat image, std::vector<Detection> detections, std::vector<std::string> classNames, std::vector<cv::Scalar> classColours)
{
    if (image.empty()) {
        std::cerr << "ERROR: Empty image provided to DrawBoundingBoxMask." << std::endl;
        return;
    }

    const int imgHeight = image.rows;
    const int imgWidth = image.cols;

    const double fontSize = std::min(imgHeight, imgWidth) * 0.0006;
    const int textThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) * 0.001));
    const float maskAlpha = 0.4f;

    cv::Mat maskImage(image.size(), image.type(), cv::Scalar::all(0));

    std::vector<const Detection*> filteredDetections;
    filteredDetections.reserve(detections.size());
    for (const auto& det : detections) {
        if (det.conf > LOCAL_CONFIDENCE_THRESHOLD &&
            det.classId >= 0 &&
            static_cast<size_t>(det.classId) < classNames.size()) {
            filteredDetections.emplace_back(&det);
        }
    }

    for (const auto* detPtr : filteredDetections) {
        cv::Rect box(detPtr->box.x, detPtr->box.y, detPtr->box.width, detPtr->box.height);
        const cv::Scalar& color = classColours.empty() ? cv::Scalar(0, 255, 0) : classColours[detPtr->classId % classColours.size()];
        cv::rectangle(maskImage, box, color, cv::FILLED);
    }

    cv::addWeighted(maskImage, maskAlpha, image, 1.0f, 0, image);

    for (const auto* detPtr : filteredDetections) {
        cv::Rect box(detPtr->box.x, detPtr->box.y, detPtr->box.width, detPtr->box.height);
        const cv::Scalar& color = classColours.empty() ? cv::Scalar(0, 255, 0) : classColours[detPtr->classId % classColours.size()];
        cv::rectangle(image, box, color, 2, cv::LINE_AA);

        std::string label = classNames[detPtr->classId] + ": " + std::to_string(static_cast<int>(detPtr->conf * 100)) + "%";
        int baseLine = 0;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, fontSize, textThickness, &baseLine);

        int labelY = std::max(detPtr->box.y, labelSize.height + 5);
        cv::Point labelTopLeft(detPtr->box.x, labelY - labelSize.height - 5);
        cv::Point labelBottomRight(detPtr->box.x + labelSize.width + 5, labelY + baseLine - 5);

        cv::rectangle(image, labelTopLeft, labelBottomRight, color, cv::FILLED);
        cv::putText(image, label, cv::Point(detPtr->box.x + 2, labelY - 2), cv::FONT_HERSHEY_SIMPLEX, fontSize, cv::Scalar(255, 255, 255), textThickness, cv::LINE_AA);
    }

    // Note: The function signature takes image by value, so changes are local.
    // If caller expects modifications, they should pass a reference; header defines image by value.
}
