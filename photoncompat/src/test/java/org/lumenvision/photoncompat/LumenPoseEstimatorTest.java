package org.lumenvision.photoncompat;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import edu.wpi.first.math.geometry.Pose3d;
import edu.wpi.first.math.geometry.Rotation3d;
import edu.wpi.first.math.geometry.Transform3d;
import edu.wpi.first.math.geometry.Translation3d;
import java.util.ArrayList;
import java.util.Optional;
import org.junit.jupiter.api.Test;

// Exercises LumenPoseEstimator.compose directly (package-private, same package) rather than
// going through the public constructor+update(): that path needs a real LumenCamera, which needs
// a real, native-backed NetworkTableInstance to even construct - LumenCamera's own live NT4 read
// path already has thorough coverage on the C++ side (test_networktables_nt4_e2e.cpp, the actual
// producer of this exact schema), and this plain java-library module (unlike the real robot/
// GradleRIO project) has no wiring to load ntcore's native JNI library in a bare test run. What's
// actually at risk of a bug here - the pose composition math, and the "no multi-tag result this
// frame -> empty" contract - needs neither a camera nor real NT4 at all.
class LumenPoseEstimatorTest {

    @Test
    void compose_appliesFieldToCameraAndCameraToRobotOffset() {
        double[] identityRowMajor = {1, 0, 0, 0, 1, 0, 0, 0, 1};
        // camera sits at field (5, 0, 0) with no rotation, having solved a 3-tag multitag PnP
        LumenMultiTagResult multiTag = new LumenMultiTagResult(5.0, 0.0, 0.0, identityRowMajor, 3, 0.4);
        LumenPipelineResult result =
                new LumenPipelineResult(new ArrayList<>(), 12.0, Optional.of(multiTag));

        // camera mounted 1m forward of the robot's own origin, no rotation - so the robot origin
        // sits 1m BEHIND the camera's own field position.
        Transform3d cameraToRobot = new Transform3d(new Translation3d(1, 0, 0), new Rotation3d()).inverse();

        Optional<LumenEstimatedRobotPose> estimated = LumenPoseEstimator.compose(result, cameraToRobot);

        assertTrue(estimated.isPresent());
        Pose3d robotPose = estimated.get().getEstimatedPose();
        assertEquals(4.0, robotPose.getX(), 1e-9);
        assertEquals(0.0, robotPose.getY(), 1e-9);
        assertEquals(0.0, robotPose.getZ(), 1e-9);
        assertEquals(12.0, estimated.get().getTimestampSeconds(), 1e-9);
        assertEquals(3, estimated.get().getTagCount());
        assertEquals(0.4, estimated.get().getReprojectionErrorPixels(), 1e-9);
    }

    @Test
    void compose_emptyWhenNoMultiTagResultThisFrame() {
        LumenPipelineResult result = new LumenPipelineResult(new ArrayList<>(), 12.0, Optional.empty());

        assertTrue(LumenPoseEstimator.compose(result, new Transform3d()).isEmpty());
    }
}
