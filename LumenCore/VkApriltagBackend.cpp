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
	// DetectorConfig's own default (0) resolves to std::thread::hardware_concurrency() inside the
	// library - every core on this 4xA76+4xA55 chip, including the 4 slow A55s, for a workload
	// that's GPU-bound with a genuinely small CPU tail (the library's own measurements: ~1.3ms at
	// 1280x800 - docs/PERFORMANCE_ANALYSIS.md's own §4). Defaulting to a fixed 4 instead avoids
	// spawning threads onto the A55 cluster at all (this project doesn't pin vkapriltag's own
	// worker pool to specific cores - a separate, deferred concern - just avoids asking for more
	// threads than the workload can use). QuadDecode's pool is sized once at construction (see
	// its own header comment - no live resize), so changing this requires rebuilding the backend,
	// same as switching CPU<->Vulkan already does. Still fully overridable per sink via
	// ApriltagTuning.nthreads (REST/webui), same as before.
	config.cpu_threads = tuning.nthreads > 0 ? static_cast<uint32_t>(tuning.nthreads) : 4;

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
