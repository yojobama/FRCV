#include "CameraCalibrationSink.h"
#include "Frame.h"
#include "Logger.h"
#include "CameraCalibrationResult.h"
#include "PreProcessor.h"
#include "ISource.h"

CameraCalibrationSink::CameraCalibrationSink(Logger* logger, PreProcessor* preProcessor, FrameSpec frameSpec) : m_FrameSpec(frameSpec)
{
	this->logger = logger;
	this->preProcessor = preProcessor;
	if (logger) logger->EnterLog("CameraCalibrationSink constructed");
    for (int i{ 0 }; i < CHECKERBOARD[1]; i++)
    {
        for (int j{ 0 }; j < CHECKERBOARD[0]; j++)
            m_Objp.push_back(cv::Point3f(j, i, 0));
    }
}

CameraCalibrationSink::~CameraCalibrationSink() 
{

};

void CameraCalibrationSink::GrabAndProcessFrame() {
    std::shared_ptr<Frame> src = m_Source->GetLatestFrame(true);
    
    FrameSpec spec(src.get()->GetSpec().GetHeight(), src.get()->GetSpec().GetWidth(), CV_8UC1);

    std::shared_ptr<Frame> gray = preProcessor->transformFrame(src, spec);

    bool success = cv::findChessboardCorners(*gray, cv::Size(CHECKERBOARD[0], CHECKERBOARD[1]), M_CornerPts, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE
    );

    if (success) {
        cv::TermCriteria criteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.001);

        // refining pixel coordinates for given 2d points.
        cv::cornerSubPix(*gray, M_CornerPts, cv::Size(11, 11), cv::Size(-1, -1), criteria);

        m_Objpoints.push_back(m_Objp);
        m_Imgpoints.push_back(M_CornerPts);
    }
}

CameraCalibrationResult CameraCalibrationSink::GetResults()
{
    cv::Mat cameraMatrix, distCoeffs, R, T;

    cv::calibrateCamera(m_Objpoints, m_Imgpoints, cv::Size(m_FrameSpec.GetWidth(), m_FrameSpec.GetHeight()), cameraMatrix, distCoeffs, R, T);

    double fx = 0.0, fy = 0.0, cx = 0.0, cy = 0.0;
    if (!cameraMatrix.empty() && cameraMatrix.rows >= 3 && cameraMatrix.cols >= 3) {
        fx = cameraMatrix.at<double>(0, 0);
        fy = cameraMatrix.at<double>(1, 1);
        cx = cameraMatrix.at<double>(0, 2);
        cy = cameraMatrix.at<double>(1, 2);
    }

    return CameraCalibrationResult(fx, fy, cx, cy, m_FrameSpec);
}

void CameraCalibrationSink::BindSource(ISource* source)
{
    this->m_Source = source;
    if (logger) logger->EnterLog("CameraCalibrationSink bound to source");
}