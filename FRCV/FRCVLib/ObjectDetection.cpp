#include "ObjectDetection.h"

/// <summary>
///  return a json string representation of the object detection
/// </summary>
/// <returns></returns>
string ObjectDetection::ToString()
{
	return "{\"class_id\": " + std::to_string(m_ClassId) +
		   ", \"class_name\": \"" + m_ClassName + "\"" +
		   ", \"confidence\": " + std::to_string(m_Confidence) +
		   ", \"bbox\": {\"x\": " + std::to_string(m_BoundingBox.x) +
		   ", \"y\": " + std::to_string(m_BoundingBox.y) +
		   ", \"width\": " + std::to_string(m_BoundingBox.width) +
		", \"height\": " + std::to_string(m_BoundingBox.height) + "}}";
}
