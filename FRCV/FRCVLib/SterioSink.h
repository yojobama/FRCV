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
#include <algorithm>
#include <vector>

class SourceBase;
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
	SterioSink(SourceBase* sourceLeft, SourceBase* sourceRight, double baseline, string EncoderName);
	~SterioSink();
	shared_ptr<Frame> getCurrentDepth();
private:
	AVFrame* frameLeft;
	AVFrame* frameRight;
	AVFrame* regularFrame;
	double baseline;
	const AVCodec* encoder;
	const AVCodec* decoder;
	AVCodecContext* encoderCtx;
	AVCodecContext* decoderCtx;
	void encodeFrames(shared_ptr<Frame> firstFrame, shared_ptr<Frame> secondFrame, EncodedPacket* output);
	void decodeAndExtractDepth(EncodedPacket* packet, const double fx, const double baseline);
	SourceBase* sourceLeft;
	SourceBase* sourceRight;
	string m_EncoderName;
};

