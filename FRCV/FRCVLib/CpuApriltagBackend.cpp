#include "CpuApriltagBackend.h"

CpuApriltagBackend::CpuApriltagBackend()
{
	m_Family = tag36h11_create();
	m_Detector = apriltag_detector_create();
	apriltag_detector_add_family(m_Detector, m_Family);
}

CpuApriltagBackend::~CpuApriltagBackend()
{
	apriltag_detector_destroy(m_Detector);
	tag36h11_destroy(m_Family);
}

zarray_t* CpuApriltagBackend::Detect(const cv::Mat& grayFrame)
{
	image_u8_t img = {
		grayFrame.cols,
		grayFrame.rows,
		grayFrame.cols,
		grayFrame.data
	};
	return apriltag_detector_detect(m_Detector, &img);
}

void CpuApriltagBackend::ReleaseResult(zarray_t* detections)
{
	apriltag_detections_destroy(detections);
}
