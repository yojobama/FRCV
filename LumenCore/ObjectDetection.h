#pragma once
#include <string>
#include <opencv2/opencv.hpp>

using namespace std;

class ObjectDetection
{
public:
	ObjectDetection() = default;
	~ObjectDetection() = default;
	string ToString();
	void SetBoundingBox(cv::Rect2d box) { m_BoundingBox = box; }
	void SetConfidence(float conf) { m_Confidence = conf; }
	void SetClassId(int id) { m_ClassId = id; }
	void SetClassName(string name) { m_ClassName = name; }
	cv::Rect2d GetBoundingBox() const { return m_BoundingBox; }
	float GetConfidence() const { return m_Confidence; }
	int GetClassId() const { return m_ClassId; }
	string GetClassName() const { return m_ClassName; }
private:
	cv::Rect2d m_BoundingBox;
	float m_Confidence;
	int m_ClassId;
	string m_ClassName;
};
