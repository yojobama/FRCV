#include "CameraCalibrator.h"

CameraCalibrator::CameraCalibrator(std::shared_ptr<Logger> logger, std::string id) : ISink(logger, 1, false, true, id), ISource(logger, id), m_Logger(logger)
{
	m_DoNotLoadCaptureThread = true;
}

CameraCalibrationResult CameraCalibrator::GetCalibrationResult()
{
	cv::Mat cameraMatrix = cv::Mat(3, 3, CV_64F);
	cv::Mat distCoeffs = cv::Mat(8, 1, CV_64F);
	std::vector<cv::Mat> rvecs, tvecs;

	double rms = cv::calibrateCamera(m_ObjPoints, m_ImgPoints, frameSize, cameraMatrix, distCoeffs, rvecs, tvecs);

	double fx = cameraMatrix.at<double>(0, 0);
	double fy = cameraMatrix.at<double>(1, 1);
	double cx = cameraMatrix.at<double>(0, 2);
	double cy = cameraMatrix.at<double>(1, 2);

    return CameraCalibrationResult(fx, fy, cx, cy, rms);
}

void CameraCalibrator::Process(std::vector<SourceResult> results)
{
	const cv::Mat& frame = results[0].frame.value();
    cv::Mat gray;

    if (frame.empty()) {
        // std::cerr << "Error: Blank frame grabbed." << std::endl;
		m_Logger->EnterLog(LogLevel::Error, "CameraCalibrator: Blank frame grabbed.");
        return;
    }

    // Create a copy to draw overlays onto without corrupting raw capture data
    cv::Mat displayFrame = frame.clone();
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // Vector to store corners found in this frame
    std::vector<cv::Point2f> corners;

    // Find internal corners
    bool patternFound = cv::findChessboardCorners(gray, patternSize, corners,
        cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);

    if (patternFound) {
        // Refine corner positions to sub-pixel accuracy
        cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
            cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));

        // Draw connecting colored lines on the display matrix
        cv::drawChessboardCorners(displayFrame, patternSize, corners, patternFound);

        cv::putText(displayFrame, "BOARD DETECTED", cv::Point(20, 40),
            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
    }
    else {
        cv::putText(displayFrame, "Searching for board...", cv::Point(20, 40),
            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
    }

    // Display counts on frame
    std::string countText = "Saved Snapshots: " + std::to_string(m_ImgPoints.size());
    cv::putText(displayFrame, countText, cv::Point(20, displayFrame.rows - 20),
        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

	SetLatestResult(SourceResult(std::nullopt, displayFrame));
}
