%module Native
%{
#include "Logger.h"
#include "IFrameSource.h"
#include "ImageFileFrameSource.h"
#include "VideoFileFrameSource.h"
#include "CameraFrameSource.h"
#include "FramePool.h"
#include "Frame.h"
#include "FrameSpec.h"
#include "ApriltagDetection.h"
#include "ApriltagSink.h"
#include "ISink.h"
#include "IObjectDetectionSink.h"
#include "RknnSink.h"
#include "TRTSink.h"
#include "PreProcessor.h"
#include "SingleSourcePipeline.h"
#include "IPipeline.h"
#include "SterioPipeline.h"
%}

%include "std_string.i"
%include "std_vector.i"

namespace std {
    %template(VectorLogger) vector<Logger>;
    %template(VectorIFrameSource) vector<IFrameSource>;
    %template(VectorImageFileFrameSource) vector<ImageFileFrameSource>;
    %template(VectorVideoFileFrameSource) vector<VideoFileFrameSource>;
    %template(VectorCameraFrameSource) vector<CameraFrameSource>;
    %template(VectorFramePool) vector<FramePool>;
    %template(VectorFrame) vector<Frame>;
    %template(VectorFrameSpec) vector<FrameSpec>;
    %template(VectorApriltagDetection) vector<ApriltagDetection>;
    %template(VectorApriltagSink) vector<ApriltagSink>;
    %template(VectorISink) vector<ISink>;
    %template(VectorIObjectDetectionSink) vector<IObjectDetectionSink>;
    %template(VectorRknnSink) vector<RknnSink>;
    %template(VectorTRTSink) vector<TRTSink>;
    %template(VectorPreProcessor) vector<PreProcessor>;
    %template(VectorSingleSourcePipeline) vector<SingleSourcePipeline>;
    %template(VectorIPipeline) vector<IPipeline>;
    %template(VectorSterioPipeline) vector<SterioPipeline>;
}

%include "Logger.h"
%include "IFrameSource.h"
%include "ImageFileFrameSource.h"
%include "VideoFileFrameSource.h"
%include "CameraFrameSource.h"
%include "FramePool.h"
%include "Frame.h"
%include "FrameSpec.h"
%include "ApriltagDetection.h"
%include "ApriltagSink.h"
%include "ISink.h"
%include "IObjectDetectionSink.h"
%include "RknnSink.h"
%include "TRTSink.h"
%include "PreProcessor.h"
%include "SingleSourcePipeline.h"
%include "IPipeline.h"
%include "SterioPipeline.h"