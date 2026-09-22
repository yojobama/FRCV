#include "CpuApriltagBackend.h"

CpuApriltagBackend::CpuApriltagBackend(int nthreads, float quadDecimate)
{
	m_Family = tag36h11_create();
	m_Detector = apriltag_detector_create();
	apriltag_detector_add_family(m_Detector, m_Family);

	// apriltag_detector_create()'s own "reasonable values" leave nthreads at 1 (single-threaded)
	// and quad_decimate at 1.0 (full-resolution quad search, the expensive setting) - neither is
	// a good default for this project's actual hardware, but both are genuinely tunable at
	// runtime (not fixed here) - see ApriltagDetector::SetTuning/SinkManager.SetApriltagBackend,
	// which rebuilds a live sink with new values exactly the way switching CPU<->Vulkan already
	// does. <= 0 means "use this backend's own library default" for either parameter.
	//
	// quad_decimate only affects the initial quad SEARCH resolution - the doc comment on this
	// field is explicit that "decoding the binary payload is still done at full resolution", so
	// a value above 1 trades a small amount of detection range for a speed win, not tag-id/pose
	// accuracy on tags actually found. refine_edges stays at its own default (true) - the apriltag
	// doc recommends it specifically to compensate for decimation's reduced quad quality, and
	// calls it "very computationally inexpensive".
	if (nthreads > 0) m_Detector->nthreads = nthreads;
	if (quadDecimate > 0.0f) m_Detector->quad_decimate = quadDecimate;
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
