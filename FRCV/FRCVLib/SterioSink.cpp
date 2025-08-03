#include "SterioSink.h"
#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>
#include "Frame.h"
#include "ISource.h"

using namespace std;

// Helper to convert cv::Mat to YUV420P AVFrame
AVFrame* convertToYUV420(const cv::Mat& bgr, SwsContext* sws, AVFrame* frame) {
    uint8_t* inData[] = { const_cast<uint8_t*>(bgr.data) };
    int inStride[] = { static_cast<int>(bgr.step[0]) };

    sws_scale(sws, inData, inStride, 0, bgr.rows, frame->data, frame->linesize);
    return frame;
}

SterioSink::SterioSink(ISource* sourceLeft, ISource* sourceRight, double baseline)
    : sourceLeft(sourceLeft), sourceRight(sourceRight), baseline(baseline) {

    encoder = avcodec_find_encoder_by_name("libx264");
    encoderCtx = avcodec_alloc_context3(encoder);
    encoderCtx->width = WIDTH;
    encoderCtx->height = HEIGHT;
    encoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    encoderCtx->time_base = { 1, 25 };
    encoderCtx->gop_size = 2;
    encoderCtx->max_b_frames = 0;

    av_opt_set(encoderCtx->priv_data, "preset", "ultrafast", 0);
    avcodec_open2(encoderCtx, encoder, nullptr);

    decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
    decoderCtx = avcodec_alloc_context3(decoder);
    decoderCtx->flags2 |= AV_CODEC_FLAG2_EXPORT_MVS;
    avcodec_open2(decoderCtx, decoder, nullptr);

    frameLeft = av_frame_alloc();
    frameRight = av_frame_alloc();
    regularFrame = av_frame_alloc();
}

SterioSink::~SterioSink() {
    av_frame_free(&frameLeft);
    av_frame_free(&frameRight);
    av_frame_free(&regularFrame);
    avcodec_free_context(&encoderCtx);
    avcodec_free_context(&decoderCtx);
}

void SterioSink::encodeFrames(shared_ptr<Frame> firstFrame, shared_ptr<Frame> secondFrame, EncodedPacket* output) {
    output->data.clear();
    output->size = 0;

    SwsContext* sws = sws_getContext(WIDTH, HEIGHT, AV_PIX_FMT_BGR24,
        WIDTH, HEIGHT, AV_PIX_FMT_YUV420P,
        0, nullptr, nullptr, nullptr);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* yuv = av_frame_alloc();
    yuv->format = AV_PIX_FMT_YUV420P;
    yuv->width = WIDTH;
    yuv->height = HEIGHT;
    av_frame_get_buffer(yuv, 0);

    for (int i = 0; i < 2; ++i) {
        auto& frame = (i == 0) ? *firstFrame : *secondFrame;
        convertToYUV420(frame, sws, yuv);
        yuv->pts = i;

        avcodec_send_frame(encoderCtx, yuv);
        while (avcodec_receive_packet(encoderCtx, pkt) == 0) {
            output->data.insert(output->data.end(), pkt->data, pkt->data + pkt->size);
            output->size += pkt->size;
            av_packet_unref(pkt);
        }
    }

    avcodec_send_frame(encoderCtx, nullptr); // flush
    while (avcodec_receive_packet(encoderCtx, pkt) == 0) {
        output->data.insert(output->data.end(), pkt->data, pkt->data + pkt->size);
        output->size += pkt->size;
        av_packet_unref(pkt);
    }

    av_frame_free(&yuv);
    av_packet_free(&pkt);
    sws_freeContext(sws);
}

void SterioSink::decodeAndExtractDepth(EncodedPacket* packet, const double fx, const double baseline) {
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = packet->data.data();
    pkt.size = static_cast<int>(packet->size);

    AVFrame* frame = av_frame_alloc();
    avcodec_send_packet(decoderCtx, &pkt);
    while (avcodec_receive_frame(decoderCtx, frame) == 0) {
        for (int i = 0; i < frame->nb_side_data; ++i) {
            if (frame->side_data[i]->type == AV_FRAME_DATA_MOTION_VECTORS) {
                auto* mvs = (AVMotionVector*)frame->side_data[i]->data;
                int count = frame->side_data[i]->size / sizeof(AVMotionVector);

                cv::Mat depth = cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC1);
                for (int j = 0; j < count; ++j) {
                    const auto& mv = mvs[j];
                    double dx = mv.dst_x - mv.src_x;
                    double dy = mv.dst_y - mv.src_y;
                    double disp = std::sqrt(dx * dx + dy * dy);
                    disp = std::max(disp, 1e-3);
                    double z = (fx * baseline) / disp;

                    int x = std::clamp((int)mv.dst_x, 0, WIDTH - 1);
                    int y = std::clamp((int)mv.dst_y, 0, HEIGHT - 1);
                    depth.at<uchar>(y, x) = std::min(255.0, z * 10.0); // naive scaling
                }
            }
        }
    }

    av_frame_free(&frame);
}

shared_ptr<Frame> SterioSink::getCurrentDepth() {
    auto left = sourceLeft->GetLatestFrame(true);
    auto right = sourceRight->GetLatestFrame(true);

    if (!left || !right) return nullptr;

    EncodedPacket packet;
    encodeFrames(left, right, &packet);

    // assuming fx is known/calibrated
    double fx = 700.0;
    decodeAndExtractDepth(&packet, fx, baseline);

    return nullptr; // placeholder: return a depth frame object if needed
}