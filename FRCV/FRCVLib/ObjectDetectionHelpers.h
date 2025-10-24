#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

struct BoundingBox
{
	int x;
	int y;
	int width;
	int height;
};
struct Detection
{
	BoundingBox box;
	float conf;
	int classId;
};

class ObjectDetectionHelpers
{
public:
	static std::vector<std::string> GetClassNames(const std::string& labelsPath);
	static unsigned long VectorProduct(const std::vector<long>& vector);
	static void LetterBox(cv::Mat& image, cv::Mat& output, cv::Size newShape, cv::Scalar colour = cv::Scalar(114, 114, 114), bool auto_ = true, bool scaleFiil = false, bool scaleUp = true, int stride = 32);
	static BoundingBox ScaleBox(const cv::Size imageShape, BoundingBox coords, cv::Size& imgOriginalShape, bool p_Clip);
	static void NMSBoxes(std::vector<BoundingBox>& boundingBoxes, std::vector<float> scores, float scoreThreashold, float nmsThreashold, std::vector<int>& indices);
	static std::vector<cv::Scalar> GenerateColours(std::vector<std::string> classNames, int seed = 42);
	static void DrawBoundingBoxes(cv::Mat& image, std::vector<Detection> detections, std::vector<std::string> classNames, std::vector<cv::Scalar> colours);
	static void DrawBoundingBoxMask(cv::Mat image, std::vector<Detection> detections, std::vector<std::string> classNames, std::vector<cv::Scalar> classColours);
};

