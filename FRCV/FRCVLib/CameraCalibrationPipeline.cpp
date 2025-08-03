
#include "CameraCalibrationPipeline.h"
#include "Frame.h"
#include "PreProcessor.h"
#include "FramePool.h"

#include <opencv2/opencv.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

CameraCalibrationPipeline::CameraCalibrationPipeline(PreProcessor* preProcessor, FramePool* framePool)
{
    this->preProcessor = preProcessor;
    this->framePool = framePool;
}

CameraCalibrationPipeline::~CameraCalibrationPipeline()
{
}

CameraCalibrationResult CameraCalibrationPipeline::getResults(vector<shared_ptr<Frame>> frames, int CHECKERBOARD[2])
{
    // Creating vector to store vectors of 3D points for each checkerboard image
    std::vector<std::vector<cv::Point3f> > objpoints;

    // Creating vector to store vectors of 2D points for each checkerboard image
    std::vector<std::vector<cv::Point2f> > imgpoints;

    // Defining the world coordinates for 3D points
    std::vector<cv::Point3f> objp;
    for (int i{ 0 }; i < CHECKERBOARD[1]; i++)
    {
        for (int j{ 0 }; j < CHECKERBOARD[0]; j++)
            objp.push_back(cv::Point3f(j, i, 0));
    }

    std::shared_ptr<Frame> gray;

    // vector to store the pixel coordinates of detected checker board corners 
    std::vector<cv::Point2f> corner_pts;

    for (auto i = frames.begin(); i < frames.end(); i++) {
        FrameSpec spec((*i)->getSpec().getHeight(), (*i)->getSpec().getWidth(), CV_8UC1);

        gray = preProcessor->transformFrame(*i, spec);

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

    cv::Mat cameraMatrix, distCoeffs, R, T;

    cv::calibrateCamera(objpoints, imgpoints, cv::Size(gray->rows, gray->cols), cameraMatrix, distCoeffs, R, T);

    double fx = 0.0, fy = 0.0, cx = 0.0, cy = 0.0;
    if (!cameraMatrix.empty() && cameraMatrix.rows >= 3 && cameraMatrix.cols >= 3) {
        fx = cameraMatrix.at<double>(0, 0);
        fy = cameraMatrix.at<double>(1, 1);
        cx = cameraMatrix.at<double>(0, 2);
        cy = cameraMatrix.at<double>(1, 2);
    }

    return CameraCalibrationResult(fx, fy, cx, cy, frameSpec);
}
