#include "FFmpegUtils.h"
std::vector<std::string> GetAvailableHardwareEncoders() {
	std::vector<std::string> hardwareEncoders;

	const AVCodec* codec = nullptr;
	void* iter = nullptr;

	while ((codec = av_codec_iterate(&iter))) {
		if (av_codec_is_encoder(codec)) {
			AVCodecContext* ctx = avcodec_alloc_context3(codec);
			if (!ctx) {
				continue;
			}

			// Minimal configuration to allow opening the codec
			ctx->width = 640;
			ctx->height = 480;
			ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			ctx->time_base = AVRational{ 1, 30 };

			int ret = avcodec_open2(ctx, codec, nullptr);
			if (ret == 0) {
				// Add codec name to the result vector if available
				if (codec->name) {
					hardwareEncoders.emplace_back(codec->name);
				}
			}

			// Free the context to avoid memory leaks
			avcodec_free_context(&ctx);
		}
	}

	return hardwareEncoders;
}
