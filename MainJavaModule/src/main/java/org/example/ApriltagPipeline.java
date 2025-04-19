package org.example;


import org.opencv.core.Mat;

public class ApriltagPipeline extends Pipeline<ApriltagDetection> {

    public ApriltagPipeline(Mat mat, FrameSpec frameSpec) {
        super(frameSpec, mat);
    }

    @Override
    ApriltagDetection internalProcessFrame() {
        // TODO implement this
        return null;
    }
}
