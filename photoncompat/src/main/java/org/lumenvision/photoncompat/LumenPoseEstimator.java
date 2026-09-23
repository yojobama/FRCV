package org.lumenvision.photoncompat;

import edu.wpi.first.math.geometry.Transform3d;

import java.util.Optional;

/**
 * Turns the coprocessor's own multi-tag PnP result into the robot's field-relative pose -
 * mirrors photonlib's {@code PhotonPoseEstimator} shape (construct once with the camera mount
 * offset, then {@code update(result)} each loop) but does far less work than it: the coprocessor
 * has already solved the multi-tag PnP itself ({@code ApriltagDetector::SolveMultiTagPnP}), so
 * this class only composes that already-solved {@link LumenMultiTagResult#getFieldToCamera()}
 * with {@code robotToCamera} - no single/multi-tag strategy selection, no ambiguity-resolution
 * heuristics, and no per-tag field layout lookup of its own (the coprocessor already has the
 * field layout loaded for its own solve).
 */
public class LumenPoseEstimator {
    private final LumenCamera camera;
    private final Transform3d cameraToRobot;

    /**
     * @param camera the coprocessor node to read multi-tag results from
     * @param robotToCamera the camera's mount offset on the robot - same convention as
     *     {@link LumenUtils#estimateFieldToRobotAprilTag}'s own {@code cameraToRobot} parameter,
     *     just inverted once here so callers supply the more commonly-measured direction
     *     (robot origin -> camera) rather than having to invert it themselves.
     */
    public LumenPoseEstimator(LumenCamera camera, Transform3d robotToCamera) {
        this.camera = camera;
        this.cameraToRobot = robotToCamera.inverse();
    }

    /** Equivalent to {@code update(camera.getLatestResult())} - the common case of reading
     * whatever the camera's own most recent snapshot is. */
    public Optional<LumenEstimatedRobotPose> update() {
        return update(camera.getLatestResult());
    }

    /**
     * @param result a snapshot from this same estimator's own {@link #camera} - passing one from
     *     a different {@link LumenCamera} produces a pose composed from the wrong coprocessor
     *     node's multi-tag result, silently.
     * @return empty exactly when {@code result}'s own {@link LumenPipelineResult#getMultiTagResult()}
     *     is empty (fewer than 2 simultaneously-visible tags with known field poses this frame).
     */
    public Optional<LumenEstimatedRobotPose> update(LumenPipelineResult result) {
        return compose(result, cameraToRobot);
    }

    // factored out as a package-private static so the actual composition math is testable
    // without a live LumenCamera (which needs a real, native-backed NetworkTableInstance to even
    // construct) - see LumenPoseEstimatorTest, which calls this directly.
    static Optional<LumenEstimatedRobotPose> compose(LumenPipelineResult result, Transform3d cameraToRobot) {
        return result.getMultiTagResult().map(multiTag -> new LumenEstimatedRobotPose(
                multiTag.getFieldToCamera().transformBy(cameraToRobot),
                result.getTimestampSeconds(),
                multiTag.getTagCount(),
                multiTag.getReprojectionErrorPixels()));
    }
}
