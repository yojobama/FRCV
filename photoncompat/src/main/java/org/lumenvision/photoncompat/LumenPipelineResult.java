package org.lumenvision.photoncompat;

import java.util.Collections;
import java.util.List;
import java.util.Optional;

/**
 * One snapshot of a coprocessor node's detections, in the shape photonlib's
 * PhotonPipelineResult already has.
 */
public class LumenPipelineResult {
    private final List<LumenTrackedTarget> targets;
    private final double timestampSeconds;
    private final Optional<LumenMultiTagResult> multiTagResult;

    LumenPipelineResult(
            List<LumenTrackedTarget> targets,
            double timestampSeconds,
            Optional<LumenMultiTagResult> multiTagResult) {
        this.targets = Collections.unmodifiableList(targets);
        this.timestampSeconds = timestampSeconds;
        this.multiTagResult = multiTagResult;
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

    /**
     * The coprocessor's own multi-tag PnP result for this same snapshot, if one was published -
     * see {@link LumenCamera#getMultiTagResult()} for when this is empty. Read as part of the
     * same NT4 poll {@link #getTargets()}/{@link #getTimestampSeconds()} came from, rather than a
     * caller doing its own separate read, so {@link LumenPoseEstimator} sees one coherent frame
     * rather than two independently-timed ones.
     */
    public Optional<LumenMultiTagResult> getMultiTagResult() {
        return multiTagResult;
    }
}
