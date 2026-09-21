#pragma once
#include <opencv2/opencv.hpp>
#include <map>
#include <string>

// One tag's known pose in FIELD coordinates (WPILib's convention: +X downfield from the blue
// alliance wall, +Y toward the left wall as viewed from the blue alliance, +Z up).
struct AprilTagFieldPose {
	cv::Point3d translation;
	cv::Matx33d rotation;
};

// Loads WPILib's own AprilTagFieldLayout JSON format directly (the same file a team's robot
// code already loads via AprilTagFieldLayout.loadFromResource(...)/fromResource(...)), rather
// than inventing a project-specific schema a team would have to hand-maintain a second copy of.
// Format (the fields this class actually reads - a real field JSON has more metadata than this,
// all ignored):
//   { "tags": [ { "ID": 1, "pose": { "translation": {"x":.., "y":.., "z":..},
//                                    "rotation": {"quaternion": {"W":..,"X":..,"Y":..,"Z":..}} }
//               }, ... ] }
//
// Used by ApriltagDetector's multi-tag PnP path (ROADMAP.md Phase 7): each visible tag with a
// known field pose contributes its 4 corners (in field-frame 3D, via AprilTagFieldPose) to one
// combined solvePnP call, recovering a single field-relative camera pose - more robust than
// trusting any one tag's own (noisier, especially at range/oblique angle) single-tag estimate.
class AprilTagFieldLayout {
public:
	// Returns false (and leaves this layout empty) on any parse failure - a missing/malformed
	// field layout file should disable multi-tag PnP gracefully, not throw and take the whole
	// detector down with it.
	bool LoadFromFile(const std::string& jsonPath);

	bool TryGetTagPose(int id, AprilTagFieldPose& outPose) const;
	size_t size() const { return m_Tags.size(); }
	bool empty() const { return m_Tags.empty(); }

private:
	std::map<int, AprilTagFieldPose> m_Tags;
};
