package org.lumenvision.photoncompat;

import java.util.Collections;
import java.util.List;

/**
 * One snapshot of a coprocessor node's detections, in the shape photonlib's
 * PhotonPipelineResult already has.
 */
public class LumenPipelineResult {
    private final List<LumenTrackedTarget> targets;
    private final double timestampSeconds;

    LumenPipelineResult(List<LumenTrackedTarget> targets, double timestampSeconds) {
        this.targets = Collections.unmodifiableList(targets);
        this.timestampSeconds = timestampSeconds;
    }

    public List<LumenTrackedTarget> getTargets() {
        return targets;
    }

    public boolean hasTargets() {
        return !targets.isEmpty();
    }

    /**
     * The moment this result was published, in the same local clock domain NT4 already
     * reconciles client/server time into (NetworkTableEntry's own timestamp - see LumenCamera's
     * own comment on why that, rather than a value read out of the JSON/NT payload itself, is
     * what a robot program should feed to a pose estimator's addVisionMeasurement).
     */
    public double getTimestampSeconds() {
        return timestampSeconds;
    }
}
