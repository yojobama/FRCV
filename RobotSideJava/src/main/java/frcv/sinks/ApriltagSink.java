package frcv.sinks;

import edu.wpi.first.apriltag.AprilTagDetection;
import frcv.FRCVSink;

public class ApriltagSink extends FRCVSink<AprilTagDetection> {
    public ApriltagSink() {
        super();
    }

    @Override
    public AprilTagDetection getDetection() {
        return null;
    }
}
