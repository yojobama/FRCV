extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/motion_vector.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <vector>
#include <string>
#include <memory>

class Frame;

namespace FFmpegUtils {
	struct MotionVectorRecord {
		int src_x, src_y;
		int dst_x, dst_y;
		int motion_x, motion_y;
		int motion_scale;
		bool is_forward;
		int block_width, block_height;
	};

	AVFrame* AllocateAVFrame(int width, int height, AVPixelFormat pix_fmt);

	void MatBGRToYUVAVFrame(std::shared_ptr<Frame>, AVFrame* frame, SwsContext*& sws);

	void SetCommonCodecParameters(AVCodecContext* codecContext, int width, int height, int fps = 30, int fps_den=1, int bitrate = 2'000'000);

	std::vector<uint8_t> EncodeTwoFrames(std::shared_ptr<Frame> leftBGR, std::shared_ptr<Frame> rightBGR, AVCodec* codec);

	//std::vector<std::string> GetAvailableVideoEncoders();
}