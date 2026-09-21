#ifdef LUMEN_WITH_VULKAN_APRILTAG
#include "VkApriltagBackend.h"
#include <apriltag/tag36h11.h>

using namespace apriltag_vulkan;

VkApriltagBackend::VkApriltagBackend(int frameWidth, int frameHeight)
{
	m_Family = tag36h11_create();
	m_Detector = apriltag_detector_create();
	apriltag_detector_add_family(m_Detector, m_Family);

	m_Context = std::make_unique<vk::Context>();

	DetectorConfig config;
	config.width = static_cast<uint32_t>(frameWidth);
	config.height = static_cast<uint32_t>(frameHeight);
	config.tag_width = static_cast<uint32_t>(m_Family->width_at_border);
	config.reversed_border = m_Family->reversed_border;
	config.normal_border = !m_Family->reversed_border;

	m_GpuDetector = std::make_unique<GpuDetector>(*m_Context, config);
	m_QuadDecode = std::make_unique<QuadDecode>(config);
	m_TagDecoder = std::make_unique<TagDecoder>(m_Detector);
}

VkApriltagBackend::~VkApriltagBackend()
{
	// destroy in reverse dependency order before m_Context (owns the device the others hold
	// live handles into) and before the detector/family the TagDecoder borrows a pointer to
	m_TagDecoder.reset();
	m_QuadDecode.reset();
	m_GpuDetector.reset();
	m_Context.reset();

	apriltag_detector_destroy(m_Detector);
	tag36h11_destroy(m_Family);
}

zarray_t* VkApriltagBackend::Detect(const cv::Mat& grayFrame)
{
	m_GpuDetector->Detect(grayFrame.data);

	std::vector<DetectedQuad> quads = m_QuadDecode->Decode(
		m_GpuDetector->last_selected_extents, m_GpuDetector->last_line_fit_points);

	return m_TagDecoder->Decode(quads, grayFrame.data,
		static_cast<uint32_t>(grayFrame.cols), static_cast<uint32_t>(grayFrame.rows),
		m_Family->reversed_border);
}

void VkApriltagBackend::ReleaseResult(zarray_t* /*detections*/)
{
	// intentionally empty - see header
}

#endif // LUMEN_WITH_VULKAN_APRILTAG
