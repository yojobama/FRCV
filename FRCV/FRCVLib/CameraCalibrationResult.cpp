#include "CameraCalibrationResult.h"

void CameraCalibrationResult::transferToApriltagInfo(apriltag_detection_info_t* dst)
{
	dst->cx = cx;
	dst->cy = cy;
	dst->fx = fx;
	dst->fy = fy;
}
