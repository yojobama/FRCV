#pragma once
#include <memory>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/motion_vector.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#include <vector>

class ISource;
class Frame;
class FramePool;
class CameraCalibrationResult;

using namespace std;

struct EncodedPacket {
	vector<uint8_t> data;
	size_t size;
};

constexpr int WIDTH = 640, HEIGHT = 480;

class SterioSink
{
public:
	SterioSink(ISource* sourceLeft, ISource* sourceRight);
	~SterioSink();
private:
	void encodeFrames(shared_ptr<Frame> firstFrame, shared_ptr<Frame> secondFrame, EncodedPacket* output);
	void decodeAndExtractDepth(EncodedPacket* packet, const double fx, const double baseline);
	ISource* sourceLeft;
	ISource* sourceRight;
};

