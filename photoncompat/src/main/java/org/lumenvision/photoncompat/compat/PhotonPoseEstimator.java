package org.lumenvision.photoncompat.compat;

import edu.wpi.first.math.geometry.Transform3d;
import org.lumenvision.photoncompat.LumenEstimatedRobotPose;
import org.lumenvision.photoncompat.LumenPoseEstimator;

import java.util.Optional;

/**
 * Thin {@link LumenPoseEstimator} wrapper - see {@link PhotonCamera}'s own class comment for why
 * this package exists. {@code update()} returns {@link LumenEstimatedRobotPose} directly (not
 * another compat-package wrapper): unlike {@code PhotonCamera}/{@code PhotonPipelineResult}/
 * {@code PhotonTrackedTarget}, real PhotonVision's own {@code EstimatedRobotPose} has no
 * {@code PoseStrategy} equivalent here worth hiding behind a second thin type - see
 * {@link LumenEstimatedRobotPose}'s own class comment.
 */
public class PhotonPoseEstimator {
    private final LumenPoseEstimator lumenEstimator;

    public PhotonPoseEstimator(PhotonCamera camera, Transform3d robotToCamera) {
        this.lumenEstimator = new LumenPoseEstimator(camera.getLumenCamera(), robotToCamera);
    }

    public Optional<LumenEstimatedRobotPose> update() {
        return lumenEstimator.update();
    }

    public Optional<LumenEstimatedRobotPose> update(PhotonPipelineResult result) {
        return lumenEstimator.update(result.getLumenPipelineResult());
    }
}
