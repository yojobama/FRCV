#ifdef LUMEN_WITH_VULKAN_APRILTAG
#include "VkApriltagBackend.h"
#include <apriltag/tag36h11.h>
#include <cmath>
#include <stdexcept>
#include <string>

using namespace apriltag_vulkan;

uint32_t VkApriltagBackend::ResolveDecimation(float requested, int frameWidth, int frameHeight)
{
	// <= 0: DetectorConfig's own default (2, this pipeline's original fixed behaviour). The GPU
	// pipeline only does integer "one representative pixel per NxN block" decimation - no
	// upstream-style 1.5x blend path - so a fractional request rounds.
	long d = requested > 0.0f ? std::lround(requested) : 2;
	if (d < 1) d = 1;
	while (d > 1 && (frameWidth % d != 0 || frameHeight % d != 0)) d--;
	return static_cast<uint32_t>(d);
}

VkApriltagBackend::VkApriltagBackend(int frameWidth, int frameHeight, ApriltagTuning tuning)
	: m_FrameWidth(frameWidth), m_FrameHeight(frameHeight)
{
	if (frameWidth <= 0 || frameHeight <= 0) {
		// the GPU pipeline's buffers are sized from these - there is no valid default
		throw std::invalid_argument("VkApriltagBackend needs the real frame size (got " +
			std::to_string(frameWidth) + "x" + std::to_string(frameHeight) + ")");
	}

	m_Family = tag36h11_create();
	m_Detector = apriltag_detector_create();
	apriltag_detector_add_family(m_Detector, m_Family);
	// set explicitly, never left to apriltag_detector_create(): it defaults refine_edges to TRUE
	// (apriltag.c), while TagDecoder's own header describes it as defaulting to false - trusting
	// either would make this knob's effect depend on which one is right.
	m_Detector->refine_edges = tuning.refineEdges;

	m_Decimation = ResolveDecimation(tuning.quadDecimate, frameWidth, frameHeight);

	m_Context = std::make_unique<vk::Context>();

	DetectorConfig config;
	config.width = static_cast<uint32_t>(frameWidth);
	config.height = static_cast<uint32_t>(frameHeight);
	config.decimation = m_Decimation;
	config.tag_width = static_cast<uint32_t>(m_Family->width_at_border);
	config.reversed_border = m_Family->reversed_border;
	config.normal_border = !m_Family->reversed_border;
	// left at DetectorConfig's own default (0 = std::thread::hardware_concurrency(), i.e. every
	// core on this 4xA76+4xA55 chip) unless this project's caller picks something smaller - a
	// single detector's CPU tail claiming all 8 cores starves whatever else the pipeline is
	// doing (another camera's own detector, capture threads, the WebRTC encoder). QuadDecode's
	// pool is sized once at construction (see its own header comment - no live resize), so
	// changing this requires rebuilding the backend, same as switching CPU<->Vulkan already does.
	if (tuning.nthreads > 0) config.cpu_threads = static_cast<uint32_t>(tuning.nthreads);

	m_GpuDetector = std::make_unique<GpuDetector>(*m_Context, config);
	m_QuadDecode = std::make_unique<QuadDecode>(config);
	// TagDecoder must be told the SAME decimation the GPU pass used: refine_edges derives its
	// per-edge search radius from it (TagDecoder sets td->quad_decimate on our behalf). kExact is
	// bit-identical to upstream's refine_edges, just without the libm modf() call - and
	// APRILTAG_VK_REFINE can still override it at runtime for benchmarking.
	m_TagDecoder = std::make_unique<TagDecoder>(m_Detector, m_Decimation, config.cpu_threads,
		RefineEdgesMethod::kExact);
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

	std::vector<DetectedQuad> quads = m_QuadDecode->Decode(m_GpuDetector->last_line_fit_points);

	return m_TagDecoder->Decode(quads, grayFrame.data,
		static_cast<uint32_t>(grayFrame.cols), static_cast<uint32_t>(grayFrame.rows),
		m_Family->reversed_border);
}

void VkApriltagBackend::ReleaseResult(zarray_t* /*detections*/)
{
	// intentionally empty - see header
}

#endif // LUMEN_WITH_VULKAN_APRILTAG
