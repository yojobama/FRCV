package org.lumenvision.photoncompat;

import edu.wpi.first.math.geometry.Pose3d;

/**
 * The robot's field-relative pose implied by one coprocessor multi-tag PnP result, produced by
 * {@link LumenPoseEstimator} - mirrors photonlib's {@code EstimatedRobotPose} shape closely
 * enough for a migrating call site to need only its import line changed, though without a {@code
 * PoseStrategy} field: unlike PhotonPoseEstimator (which can fall back across several strategies
 * when multi-tag data isn't available), this always comes from exactly one source - the
 * coprocessor's own {@link LumenMultiTagResult} - so there is no strategy to report.
 */
public class LumenEstimatedRobotPose {
    private final Pose3d estimatedPose;
    private final double timestampSeconds;
    private final int tagCount;
    private final double reprojectionErrorPixels;

    LumenEstimatedRobotPose(Pose3d estimatedPose, double timestampSeconds, int tagCount, double reprojectionErrorPixels) {
        this.estimatedPose = estimatedPose;
        this.timestampSeconds = timestampSeconds;
        this.tagCount = tagCount;
        this.reprojectionErrorPixels = reprojectionErrorPixels;
    }

    /** The robot's own field-relative pose - {@link LumenMultiTagResult#getFieldToCamera()}
     * composed with the {@code robotToCamera} transform {@link LumenPoseEstimator} was
     * constructed with. */
    public Pose3d getEstimatedPose() {
        return estimatedPose;
    }

    /** Same clock domain as {@link LumenPipelineResult#getTimestampSeconds()} - safe to pass
     * straight to a WPILib pose estimator's {@code addVisionMeasurement}. */
    public double getTimestampSeconds() {
        return timestampSeconds;
    }

    /** How many tags contributed to the underlying multi-tag solve - always >= 2. */
    public int getTagCount() {
        return tagCount;
    }

    /** RMS reprojection error in pixels - see {@link LumenMultiTagResult#getReprojectionErrorPixels()}. */
    public double getReprojectionErrorPixels() {
        return reprojectionErrorPixels;
    }
}
