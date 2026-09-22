#pragma once
#include <opencv2/opencv.hpp>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

// Recycles full-frame-sized pixel buffers instead of malloc/free-ing a fresh one on every
// capture/detection cycle. Frame.h's own history: an earlier "FramePool" existed and was deleted
// (see git history / ROADMAP.md), and Frame was rebuilt afterward deliberately WITHOUT pooling -
// "for format and ownership, not object pooling" - because at the time nothing had confirmed
// pooling was actually worth the complexity. It is: every V4L2 capture (MJPEG decode, YUYV/GREY
// colour conversion), every AprilTag/object-detection annotation clone, and every Frame::AsGray/
// AsBgr lazy conversion allocated a brand new buffer, every single frame, with nothing reused -
// real allocator churn and cache pressure on the Pi's ARM cores under sustained load. This is
// that, done properly this time: scoped narrowly (only full-frame image buffers opt in, not
// every stray small cv::Mat in the codebase - calibration matrices etc. never go anywhere near
// this), and built on plain shared_ptr refcounting rather than a custom cv::MatAllocator, which
// would have to duplicate OpenCV's own UMatData lifecycle rules to be safe - a much larger
// surface to get subtly wrong in code that runs on every frame of a live robot vision pipeline.
//
// Thread-safety: Acquire() is safe to call concurrently from any thread (the capture thread and
// every bound sink's own processing thread all reach into the same process-wide pool). A buffer
// returns to the pool automatically, exactly once, when the LAST reference to it is released -
// ordinary shared_ptr refcounting, not something this class tracks by hand. A frame held by three
// different sink threads at once (a real, routine case - GetLatestResult() hands out a copy of
// the SourceResult, and Frame is refcounted-shared, not cloned, on copy) is safe: the buffer
// isn't recycled until all three, plus whoever originally acquired it, have let go.
class FramePool
{
public:
	static FramePool& Instance();

	// Returns a cv::Mat of exactly this shape. Recycled from the free list when a buffer of the
	// same rows/cols/type is available, freshly allocated otherwise (the first frame at any given
	// resolution always allocates - same as before this existed). `owner` receives the buffer's
	// lifetime handle: keep it alive for exactly as long as the returned Mat is in use (Frame's
	// constructor does this by holding it alongside the Mat - see Frame.h's own m_PoolOwner).
	// The returned Mat does NOT own/refcount its own pixel data the way a normally-`create()`d
	// Mat does (it wraps `owner`'s buffer via the raw-pointer constructor) - `.clone()`ing it, or
	// handing it to something that expects a self-contained Mat, works fine either way; it's only
	// THIS Mat instance (and further Mats built from it via cv::Mat's own copy, which shares the
	// same header) that depends on `owner` outliving it.
	cv::Mat Acquire(int rows, int cols, int type, std::shared_ptr<void>& owner);

private:
	struct Key {
		int rows, cols, type;
		bool operator==(const Key& other) const {
			return rows == other.rows && cols == other.cols && type == other.type;
		}
	};
	struct KeyHash {
		size_t operator()(const Key& k) const {
			return (static_cast<size_t>(k.rows) * 73856093u) ^
			       (static_cast<size_t>(k.cols) * 19349663u) ^
			       (static_cast<size_t>(k.type) * 83492791u);
		}
	};

	std::mutex m_Mutex;
	std::unordered_map<Key, std::vector<std::shared_ptr<std::vector<uint8_t>>>, KeyHash> m_Free;
};
