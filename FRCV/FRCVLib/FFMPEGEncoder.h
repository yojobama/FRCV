#pragma once
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <execution>

class FFmpegEncoder {
public:
    FFmpegEncoder(int width, int height, int fps) : width(width), height(height), fps(fps) {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) {
            throw std::runtime_error("H.264 codec not found");
        }

        codecContext = avcodec_alloc_context3(codec);
        codecContext->width = width;
        codecContext->height = height;
        codecContext->time_base = { 1, fps };
        codecContext->framerate = { fps, 1 };
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        codecContext->bit_rate = 2000000;

        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
            throw std::runtime_error("Failed to open codec");
        }
    }

    ~FFmpegEncoder() {
        avcodec_free_context(&codecContext);
    }

    void encodeFrame(const AVFrame* frame) {
        AVPacket packet;
        av_init_packet(&packet);
        packet.data = nullptr;
        packet.size = 0;

        int ret = avcodec_send_frame(codecContext, frame);
        if (ret < 0) {
            throw std::runtime_error("Failed to send frame to encoder");
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(codecContext, &packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                throw std::runtime_error("Error during encoding");
            }

            // Process encoded packet (e.g., send over network)
            av_packet_unref(&packet);
        }
    }

private:
    int width, height, fps;
    const AVCodec* codec = nullptr;
    AVCodecContext* codecContext = nullptr;
};