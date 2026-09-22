package org.lumenvision.photoncompat;

import static org.junit.jupiter.api.Assertions.assertEquals;

import edu.wpi.first.math.geometry.Pose2d;
import edu.wpi.first.math.geometry.Pose3d;
import edu.wpi.first.math.geometry.Rotation2d;
import edu.wpi.first.math.geometry.Rotation3d;
import edu.wpi.first.math.geometry.Transform2d;
import edu.wpi.first.math.geometry.Transform3d;
import edu.wpi.first.math.geometry.Translation2d;
import edu.wpi.first.math.geometry.Translation3d;
import org.junit.jupiter.api.Test;

// Real, hand-verified geometry cases, not just "does it compile" - LumenUtils.
// estimateCameraToTarget's first draft compiled cleanly and looked plausible but put the camera
// on the wrong side of the target entirely (traced by hand: expected x=5, computed x=15). These
// pin every formula against an independently-derived expected answer.
class LumenUtilsTest {

    private static final double EPS = 1e-9;

    @Test
    void estimateFieldToRobotAprilTag_directlyAheadNoRotation() {
        // tag at field (5,0,0), camera sees it 3m straight ahead with no relative rotation and
        // no mount offset - the camera (and robot) must be at field x = 5 - 3 = 2.
        Pose3d fieldToTarget = new Pose3d(new Translation3d(5, 0, 0), new Rotation3d());
        Transform3d cameraToTarget = new Transform3d(new Translation3d(3, 0, 0), new Rotation3d());
        Transform3d cameraToRobot = new Transform3d();

        Pose3d fieldToRobot = LumenUtils.estimateFieldToRobotAprilTag(cameraToTarget, fieldToTarget, cameraToRobot);

        assertEquals(2.0, fieldToRobot.getX(), EPS);
        assertEquals(0.0, fieldToRobot.getY(), EPS);
        assertEquals(0.0, fieldToRobot.getZ(), EPS);
    }

    @Test
    void estimateFieldToRobotAprilTag_withCameraMountOffset() {
        // same as above, but the camera is mounted 0.5m forward of the robot's own origin - the
        // robot origin must end up 0.5m further back than the camera.
        Pose3d fieldToTarget = new Pose3d(new Translation3d(5, 0, 0), new Rotation3d());
        Transform3d cameraToTarget = new Transform3d(new Translation3d(3, 0, 0), new Rotation3d());
        Transform3d cameraToRobot = new Transform3d(new Translation3d(-0.5, 0, 0), new Rotation3d());

        Pose3d fieldToRobot = LumenUtils.estimateFieldToRobotAprilTag(cameraToTarget, fieldToTarget, cameraToRobot);

        assertEquals(1.5, fieldToRobot.getX(), EPS);
    }

    @Test
    void estimateCameraToTarget_directlyAheadNoRotation() {
        // target dead ahead (yaw 0) at distance 5, target facing back toward -X (180deg,
        // pointing at an approaching robot), robot's own gyro at 0deg. Camera-to-target's
        // translation must equal the input translation directly, and its rotation must be the
        // target's rotation minus the robot's own heading (180 - 0 = 180).
        Translation2d cameraToTargetTranslation = new Translation2d(5, 0);
        Pose2d targetPose = new Pose2d(10, 0, Rotation2d.k180deg);
        Rotation2d gyroAngle = Rotation2d.kZero;

        Transform2d cameraToTarget = LumenUtils.estimateCameraToTarget(cameraToTargetTranslation, targetPose, gyroAngle);

        assertEquals(5.0, cameraToTarget.getX(), EPS);
        assertEquals(0.0, cameraToTarget.getY(), EPS);
        assertEquals(Math.PI, Math.abs(cameraToTarget.getRotation().getRadians()), EPS);
    }

    @Test
    void estimateFieldToRobot_2d_matchesHandDerivedPosition() {
        // chained end to end: the same scenario as estimateCameraToTarget's own test above,
        // composed with estimateFieldToRobot - the camera (and, with zero mount offset, the
        // robot) must land at field x = 10 - 5 = 5, facing 0deg (matching the gyro reading fed
        // in above, which is exactly what a correct round trip must reproduce).
        Translation2d cameraToTargetTranslation = new Translation2d(5, 0);
        Pose2d targetPose = new Pose2d(10, 0, Rotation2d.k180deg);
        Rotation2d gyroAngle = Rotation2d.kZero;
        Transform2d cameraToTarget = LumenUtils.estimateCameraToTarget(cameraToTargetTranslation, targetPose, gyroAngle);

        Pose2d fieldToRobot = LumenUtils.estimateFieldToRobot(cameraToTarget, targetPose, new Transform2d());

        assertEquals(5.0, fieldToRobot.getX(), EPS);
        assertEquals(0.0, fieldToRobot.getY(), EPS);
        assertEquals(0.0, fieldToRobot.getRotation().getRadians(), EPS);
    }

    @Test
    void getYawToPose_isFieldRelativeIndependentOfRobotHeading() {
        // target at (5,5) relative to a robot at the origin: absolute field bearing is 45deg
        // regardless of which way the robot itself is currently facing - checked at two
        // different robot headings to confirm the formula doesn't accidentally depend on it.
        Pose2d targetPose = new Pose2d(5, 5, Rotation2d.kZero);

        Rotation2d bearingFacingZero =
                LumenUtils.getYawToPose(new Pose2d(0, 0, Rotation2d.kZero), targetPose);
        Rotation2d bearingFacingNinety =
                LumenUtils.getYawToPose(new Pose2d(0, 0, Rotation2d.kCCW_Pi_2), targetPose);

        assertEquals(Math.PI / 4, bearingFacingZero.getRadians(), EPS);
        assertEquals(Math.PI / 4, bearingFacingNinety.getRadians(), EPS);
    }

    @Test
    void calculateDistanceToTargetMeters_matchesKnownTriangle() {
        // camera at 0.5m height, target at 2.5m height, camera pitched up 0deg, target measured
        // at 45deg pitch in the image - a 45-45-90 triangle, so horizontal distance equals the
        // height difference.
        double distance = LumenUtils.calculateDistanceToTargetMeters(0.5, 2.5, 0.0, Math.PI / 4);
        assertEquals(2.0, distance, EPS);
    }
}
