#include "CpuApriltagBackend.h"

CpuApriltagBackend::CpuApriltagBackend(ApriltagTuning tuning)
{
	m_Family = tag36h11_create();
	m_Detector = apriltag_detector_create();
	apriltag_detector_add_family(m_Detector, m_Family);

	// apriltag_detector_create()'s own "reasonable values" are nthreads=1 (single-threaded),
	// quad_decimate=2.0 and refine_edges=true. All three are genuinely tunable at runtime (not
	// fixed here) - see SinkManager.SetApriltagBackend, which rebuilds a live sink with new values
	// exactly the way switching CPU<->Vulkan already does. <= 0 for nthreads/quadDecimate means
	// "use this backend's own library default".
	//
	// quad_decimate only affects the initial quad SEARCH resolution - the doc comment on this
	// field is explicit that "decoding the binary payload is still done at full resolution", so
	// a value above 1 trades a small amount of detection range for a speed win, not tag-id/pose
	// accuracy on tags actually found. refine_edges is recommended by the apriltag docs
	// specifically to compensate for decimation's reduced quad quality ("very computationally
	// inexpensive"); switching it off is for a measured speed win on a tight CPU budget.
	if (tuning.nthreads > 0) m_Detector->nthreads = tuning.nthreads;
	if (tuning.quadDecimate > 0.0f) m_Detector->quad_decimate = tuning.quadDecimate;
	m_Detector->refine_edges = tuning.refineEdges;
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
