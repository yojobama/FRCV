package org.lumenvision.lib.sinks;

import edu.wpi.first.apriltag.AprilTagDetection;
import org.lumenvision.lib.LumenSink;

public class ApriltagSink extends LumenSink<AprilTagDetection> {
    public ApriltagSink() {
        super();
    }

    @Override
    public AprilTagDetection getDetection() {
        return null;
    }
}
