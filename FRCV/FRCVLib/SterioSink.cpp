#include "SterioSink.h"
#include <opencv2/opencv.hpp>
#include "Frame.h"
#include "CameraCalibrationResult.h"

void SterioSink::encodeFrames(shared_ptr<Frame> firstFrame, shared_ptr<Frame> secondFrame, EncodedPacket* output)
{
    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    ctx->width = WIDTH;
    ctx->height = HEIGHT;
    ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    ctx->time_base = { 1, 25 };
    ctx->gop_size = 2;
    ctx->max_b_frames = 0;

    av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
    avcodec_open2(ctx, codec, nullptr);

    SwsContext* sws = sws_getContext(WIDTH, HEIGHT, AV_PIX_FMT_BGR24,
        WIDTH, HEIGHT, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);

    AVFrame* frame1 = av_frame_alloc();
    AVFrame* frame2 = av_frame_alloc();
    frame1->format = AV_PIX_FMT_YUV420P;
    frame1->width = WIDTH;
    frame1->height = HEIGHT;
    av_frame_get_buffer(frame1, 0);

    frame2->format = AV_PIX_FMT_YUV420P;
    frame2->width = WIDTH;
    frame2->height = HEIGHT;
    av_frame_get_buffer(frame2, 0);

    AVPacket* pkt = av_packet_alloc();
    shared_ptr<Frame> bgr1 = firstFrame;
    shared_ptr<Frame> bgr2 = secondFrame;
    uint8_t* src1[] = { bgr1->data };
    uint8_t* src2[] = { bgr2->data };
    int src_stride1[] = { static_cast<int>(bgr1->step[0]) };
    int src_stride2[] = { static_cast<int>(bgr2->step[0]) };
    sws_scale(sws, src1, src_stride1, 0, HEIGHT, frame1->data, frame1->linesize);
    sws_scale(sws, src2, src_stride2, 0, HEIGHT, frame2->data, frame2->linesize);
    frame1->pts = 0;
    frame2->pts = 1;

    avcodec_send_frame(ctx, frame1);
    avcodec_send_frame(ctx, frame2);
    while (avcodec_receive_packet(ctx, pkt) == 0) {
        output->data.insert(output->data.end(), pkt->data, pkt->data + pkt->size);
        output->size += pkt->size;
        av_packet_unref(pkt);
    }

    avcodec_send_frame(ctx, nullptr);  // flush
    while (avcodec_receive_packet(ctx, pkt) == 0) {
        output->data.insert(output->data.end(), pkt->data, pkt->data + pkt->size);
        output->size += pkt->size;
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    av_frame_free(&frame1);
    av_frame_free(&frame2);
    sws_freeContext(sws);
    avcodec_free_context(&ctx);
}

void SterioSink::decodeAndExtractDepth(EncodedPacket* packet, const double fx, const double baseline)
{
    const AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
    AVCodecContext* dec_ctx = avcodec_alloc_context3(decoder);
    dec_ctx->flags2 |= AV_CODEC_FLAG2_EXPORT_MVS;
    avcodec_open2(dec_ctx, decoder, nullptr);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    pkt->data = const_cast<uint8_t*>(packet->data.data());
    pkt->size = static_cast<int>(packet->size);

    avcodec_send_packet(dec_ctx, pkt);
    while (avcodec_receive_frame(dec_ctx, frame) == 0) {
        for (int i = 0; i < frame->nb_side_data; ++i) {
            if (frame->side_data[i]->type == AV_FRAME_DATA_MOTION_VECTORS) {
                auto* mvs = (AVMotionVector*)frame->side_data[i]->data;
                int count = frame->side_data[i]->size / sizeof(AVMotionVector);

                cv::Mat depth(HEIGHT, WIDTH, CV_8UC1, cv::Scalar(0));
                for (int j = 0; j < count; ++j) {
                    const AVMotionVector& mv = mvs[j];
                    int x = mv.dst_x;
                    int y = mv.dst_y;
                    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) continue;

                    double dx = mv.dst_x - mv.src_x;
                    double dy = mv.dst_y - mv.src_y;
                    double disparity = std::sqrt(dx * dx + dy * dy);
                    disparity = std::max(1e-3, disparity);

                    double depth_m = (fx * baseline) / disparity;
                    int grayscale = std::min(255, static_cast<int>(255.0 * depth_m / 5.0));
                    depth.at<uchar>(y, x) = grayscale;
                }

                cv::imshow("Depth Estimate", depth);
                cv::waitKey(0);
            }
        }
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&dec_ctx);
}
