#include "CameraCalibrationSink.h"
#include "Frame.h"
#include "Logger.h"
#include "CameraCalibrationResult.h"
#include "PreProcessor.h"
#include "ISource.h"

CameraCalibrationSink::CameraCalibrationSink(Logger* logger, PreProcessor* preProcessor, FrameSpec frameSpec) : frameSpec(frameSpec)
{
	this->logger = logger;
	this->preProcessor = preProcessor;
	if (logger) logger->enterLog("CameraCalibrationSink constructed");
    for (int i{ 0 }; i < CHECKERBOARD[1]; i++)
    {
        for (int j{ 0 }; j < CHECKERBOARD[0]; j++)
            objp.push_back(cv::Point3f(j, i, 0));
    }
}

CameraCalibrationSink::~CameraCalibrationSink() 
{

};

void CameraCalibrationSink::grabAndProcessFrame() {
    std::shared_ptr<Frame> src = source->getLatestFrame(true);
    
    FrameSpec spec(src.get()->getSpec().getHeight(), src.get()->getSpec().getWidth(), CV_8UC1);

    std::shared_ptr<Frame> gray = preProcessor->transformFrame(src, spec);

    bool success = cv::findChessboardCorners(*gray, cv::Size(CHECKERBOARD[0], CHECKERBOARD[1]), corner_pts, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE
    );

    if (success) {
        cv::TermCriteria criteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.001);

        // refining pixel coordinates for given 2d points.
        cv::cornerSubPix(*gray, corner_pts, cv::Size(11, 11), cv::Size(-1, -1), criteria);

        objpoints.push_back(objp);
        imgpoints.push_back(corner_pts);
    }
}

CameraCalibrationResult CameraCalibrationSink::getResults()
{
    cv::Mat cameraMatrix, distCoeffs, R, T;

    cv::calibrateCamera(objpoints, imgpoints, cv::Size(frameSpec.getWidth(), frameSpec.getHeight()), cameraMatrix, distCoeffs, R, T);

    double fx = 0.0, fy = 0.0, cx = 0.0, cy = 0.0;
    if (!cameraMatrix.empty() && cameraMatrix.rows >= 3 && cameraMatrix.cols >= 3) {
        fx = cameraMatrix.at<double>(0, 0);
        fy = cameraMatrix.at<double>(1, 1);
        cx = cameraMatrix.at<double>(0, 2);
        cy = cameraMatrix.at<double>(1, 2);
    }

    return CameraCalibrationResult(fx, fy, cx, cy, frameSpec);
}

void CameraCalibrationSink::bindSource(ISource* source)
{
    this->source = source;
    if (logger) logger->enterLog("CameraCalibrationSink bound to source");
}