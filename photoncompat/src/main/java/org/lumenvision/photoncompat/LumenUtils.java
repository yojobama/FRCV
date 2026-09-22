package org.lumenvision.photoncompat;

import edu.wpi.first.math.geometry.Pose2d;
import edu.wpi.first.math.geometry.Pose3d;
import edu.wpi.first.math.geometry.Rotation2d;
import edu.wpi.first.math.geometry.Transform2d;
import edu.wpi.first.math.geometry.Transform3d;
import edu.wpi.first.math.geometry.Translation2d;

/**
 * Static helpers for turning a {@link LumenTrackedTarget} into robot/field poses, mirroring
 * photonlib's {@code PhotonUtils} (same method names/signatures) so a team migrating from
 * PhotonVision changes an import line, not their aiming/pose-estimation code.
 *
 * <p>{@link #estimateFieldToRobotAprilTag} is the one most robot programs actually want: unlike
 * the 2D pitch/yaw helpers below (which assume a target's height and the camera's mount angle
 * are known constants, then solve a single distance+bearing triangle), it consumes {@link
 * LumenTrackedTarget#getBestCameraToTarget()} directly - the coprocessor's own solvePnP result,
 * already a full 3D transform - and needs no separate distance/angle bookkeeping at all.
 */
public final class LumenUtils {
    private LumenUtils() {}

    /**
     * Distance from the camera to a target, purely from mount geometry and the target's
     * measured pitch - the classic "known target height" triangle every FRC vision tutorial
     * starts with. Prefer {@link #estimateFieldToRobotAprilTag} when a full 3D
     * {@link Transform3d} is already available (any {@link LumenTrackedTarget}), since that
     * needs no separately-measured camera pitch/height at all.
     *
     * @param cameraHeightMeters height of the camera's lens off the floor
     * @param targetHeightMeters height of the target off the floor
     * @param cameraPitchRadians camera mount angle above horizontal (positive = tilted up)
     * @param targetPitchRadians the target's measured pitch in the camera's image (positive = up)
     */
    public static double calculateDistanceToTargetMeters(
            double cameraHeightMeters,
            double targetHeightMeters,
            double cameraPitchRadians,
            double targetPitchRadians) {
        return (targetHeightMeters - cameraHeightMeters)
                / Math.tan(cameraPitchRadians + targetPitchRadians);
    }

    /** The camera-to-target translation implied by a known distance and measured yaw. */
    public static Translation2d estimateCameraToTargetTranslation(
            double targetDistanceMeters, Rotation2d yaw) {
        return new Translation2d(targetDistanceMeters, yaw);
    }

    /**
     * Composes a camera-to-target translation with the target's known field pose and the
     * robot's gyro heading into a camera-to-target {@link Transform2d} - the 2D-only path (no
     * target roll/pitch, no camera mount transform); most callers with a real
     * {@link LumenTrackedTarget} should reach for {@link #estimateFieldToRobotAprilTag} instead.
     *
     * <p>cameraToTargetTranslation is already the target's position expressed in the camera's
     * own local frame (that's what "camera-to-target" means), so it becomes the Transform2d's
     * translation directly; the target's rotation relative to the camera is the one piece that
     * needs computing, by subtracting the camera's own field-relative heading (gyroAngle) from
     * the target's known field-relative rotation.
     */
    public static Transform2d estimateCameraToTarget(
            Translation2d cameraToTargetTranslation, Pose2d targetPose, Rotation2d gyroAngle) {
        return new Transform2d(cameraToTargetTranslation, targetPose.getRotation().minus(gyroAngle));
    }

    /**
     * The field-relative robot pose implied by one AprilTag detection's full 3D pose - the
     * direct, no-separate-measurements path: {@code cameraToTarget} is
     * {@link LumenTrackedTarget#getBestCameraToTarget()} (the coprocessor's own solvePnP
     * result), {@code fieldToTarget} is that tag's known field pose (from a field layout - see
     * ROADMAP.md Phase 7's multi-tag PnP note for why a single-tag estimate like this one is
     * more sensitive to a distant/oblique tag's pose noise than a true multi-tag solve), and
     * {@code cameraToRobot} is the camera's own mount offset on the robot.
     */
    public static Pose3d estimateFieldToRobotAprilTag(
            Transform3d cameraToTarget, Pose3d fieldToTarget, Transform3d cameraToRobot) {
        return fieldToTarget.transformBy(cameraToTarget.inverse()).transformBy(cameraToRobot);
    }

    /** The 2D robot pose implied by a 2D camera-to-target transform, the target's known field
     * pose, and the camera's own mount offset on the robot. */
    public static Pose2d estimateFieldToRobot(
            Transform2d cameraToTarget, Pose2d fieldToTarget, Transform2d cameraToRobot) {
        return fieldToTarget.transformBy(cameraToTarget.inverse()).transformBy(cameraToRobot);
    }

    /** The bearing from the robot's current pose to a target's field pose - useful for a simple
     * turn-to-face-target rotation setpoint independent of the camera's own instantaneous yaw
     * reading. */
    public static Rotation2d getYawToPose(Pose2d robotPose, Pose2d targetPose) {
        Translation2d relativeTrl = targetPose.relativeTo(robotPose).getTranslation();
        return new Rotation2d(relativeTrl.getX(), relativeTrl.getY()).plus(robotPose.getRotation());
    }
}
