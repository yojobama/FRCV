#ifdef LUMEN_WITH_VULKAN_APRILTAG
#include "VkApriltagBackend.h"
#include <apriltag/tag36h11.h>

using namespace apriltag_vulkan;

VkApriltagBackend::VkApriltagBackend(int frameWidth, int frameHeight, int cpuThreads)
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
	// left at DetectorConfig's own default (0 = std::thread::hardware_concurrency(), i.e. every
	// core on this 4xA76+4xA55 chip) unless this project's caller picks something smaller - a
	// single detector's CPU tail claiming all 8 cores starves whatever else the pipeline is
	// doing (another camera's own detector, capture threads, the WebRTC encoder). QuadDecode's
	// pool is sized once at construction (see its own header comment - no live resize), so
	// changing this requires rebuilding the backend, same as switching CPU<->Vulkan already does.
	if (cpuThreads > 0) config.cpu_threads = static_cast<uint32_t>(cpuThreads);

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
