#pragma once
#include <string>

// Implemented by any node whose two bound sources have a fixed left/right meaning
// (StereoCalibrator, StereoDepthNode). ISink::BindSource's own bookkeeping (m_Sources, a plain
// vector) is bind-order only - it has no notion of "which one is left" and nothing stops a
// caller from unbinding/rebinding in a different order later. Getting left/right backwards
// flips the sign of every disparity, which then presents as "every block invalid" (min_disparity
// gates out the negative side) rather than as an obvious error - see STEREO_IMPLEMENTATION_PLAN
// P2. Manager::BindStereoSources binds via the normal ISink::BindSource path (for lifecycle/
// registration) AND separately dynamic_casts to this interface to record the explicit roles by
// source ID, so pairing is correct regardless of bind order or a later rebind.
class IStereoRoleReceiver {
public:
	virtual ~IStereoRoleReceiver() = default;
	virtual void SetStereoRoles(const std::string& leftSourceId, const std::string& rightSourceId) = 0;
};
