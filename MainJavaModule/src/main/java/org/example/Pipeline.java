package org.example;

import org.opencv.core.Mat;
import org.opencv.imgproc.Imgproc;

public abstract class Pipeline<TResult> {

    FrameSpec frameSpec;
    Mat frame;
    boolean isProcessing;

    public Pipeline(FrameSpec spec, Mat frame) {
        this.frameSpec = spec;
        this.frame = frame;
    }

    public Mat getFrame() {
        return frame;
    }

    public boolean getIsProcessing() {
        return isProcessing;
    }

    public TResult processFrame() {
        try {
            isProcessing = true;
            PreProcessFrame();
            return internalProcessFrame();
        } finally {
            isProcessing = false;
        }
    }

    abstract TResult internalProcessFrame();

    private void PreProcessFrame() {
        // create a new frame spec based on the original frame
        FrameSpec startSpec = new FrameSpec(frame.width(), frame.height(), frame.type());

        // check if the frame needs to be resized or converted
        boolean widthChanged = frameSpec.width() != startSpec.width();
        boolean heightChanged = frameSpec.height() != startSpec.height();
        boolean typeChanged = frameSpec.frameType() != startSpec.frameType();

        // resize the frame if required
        if (widthChanged || heightChanged) {
            Imgproc.resize(frame, frame, new org.opencv.core.Size(frameSpec.width(), frameSpec.height()));
        }

        // convert to the required frame type if required
        if (typeChanged) {
            frame.convertTo(frame, frameSpec.frameType());
        }
    }
}
