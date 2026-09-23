#pragma once

#include "ISink.h"
#include <mutex>
#include <string>

// ROADMAP.md Phase 8/E7: a fallback preview stream simpler than WebRTCSink - no H.264 encode, no
// signalling, just a plain per-frame JPEG any browser's own <img src="..."> tag (or curl) can
// render/read with zero client-side code, at the cost of no compression across frames and no
// audio/data-channel machinery WebRTCSink has. Terminal sink: binds to any single frame-producing
// node (raw camera, or a detector's annotated output), matching WebRTCSink's own single-source
// convention.
//
// Always available (no LUMEN_WITH_* guard) - unlike WebRTCSink/NetworkTablesSink, this needs no
// optional third-party library (libdatachannel/ntcore) that might not be compiled in; it's pure
// OpenCV, the same floor dependency every build of this project already has.
class MjpegSink : public ISink
{
public:
	// jpegQuality is cv::IMWRITE_JPEG_QUALITY's own 0-100 scale.
	MjpegSink(std::shared_ptr<Logger> logger, std::string id, int jpegQuality = 80);

	// The most recently encoded frame, base64-encoded - not raw bytes: this crosses the C++/C#
	// SWIG boundary through the same plain std::string typemap every other string-returning
	// method in this class already uses safely. That typemap assumes printable/null-terminated
	// text (confirmed against SWIG's own std_string.i for C#, which marshals through a
	// UTF8-string conversion) - raw JPEG bytes contain arbitrary values, including embedded
	// nulls, that would silently truncate or corrupt crossing that same boundary unencoded.
	// Base64's ~33% size overhead is a real, deliberate tradeoff for a stream whose whole point
	// is being the simple/low-effort fallback, not the performant path (that's WebRTCSink).
	// Empty until the first frame has actually been processed.
	std::string GetLatestJpegBase64() const;

private:
	void Process(const std::vector<SourceResult>& results) override;

	int m_JpegQuality;
	// guards m_LatestJpegBase64 - written from this sink's own processing thread (Process()),
	// read from whichever thread handles an incoming HTTP request for it (MjpegStreamModule.cs,
	// via Manager::GetMjpegFrameBase64).
	mutable std::mutex m_Mutex;
	std::string m_LatestJpegBase64;
};
