package org.lumenvision.photoncompat;

import edu.wpi.first.math.Matrix;
import edu.wpi.first.math.geometry.Pose3d;
import edu.wpi.first.math.geometry.Rotation3d;
import edu.wpi.first.math.geometry.Translation3d;
import edu.wpi.first.math.numbers.N3;
import org.ejml.simple.SimpleMatrix;

/**
 * The coprocessor's own multi-tag PnP result (ROADMAP.md Phase 7): one field-relative camera
 * pose jointly solved from every simultaneously-visible tag with a known field pose, rather than
 * any single tag's own (noisier, especially at range/oblique angle) estimate. Mirrors photonlib's
 * MultiTargetPNPResult in spirit, though not its exact shape - this project's coprocessor already
 * inverts the solve into a direct field-relative camera {@link Pose3d} (photonlib publishes the
 * raw camera-to-field transform and leaves the inversion to callers).
 */
public class LumenMultiTagResult {
    private final Pose3d fieldToCamera;
    private final int tagCount;
    private final double reprojectionErrorPixels;

    /**
     * @param rotationRowMajor the 9 elements of the camera's own field-relative rotation matrix,
     *     row-major - same convention as {@link LumenTrackedTarget}'s own rotation matrix, for
     *     the same reason (NetworkTablesSink.cpp's tags/r0..r8 topics): a robot program builds
     *     the one Transform3d/Pose3d conversion itself, with no lossy Euler/quaternion
     *     intermediate this end could get a convention wrong on.
     */
    LumenMultiTagResult(double x, double y, double z, double[] rotationRowMajor, int tagCount, double reprojectionErrorPixels) {
        Matrix<N3, N3> rotationMatrix = new Matrix<>(new SimpleMatrix(3, 3, true, rotationRowMajor));
        this.fieldToCamera = new Pose3d(new Translation3d(x, y, z), new Rotation3d(rotationMatrix));
        this.tagCount = tagCount;
        this.reprojectionErrorPixels = reprojectionErrorPixels;
    }

    /**
     * The camera's own pose in FIELD coordinates - not camera-to-tag, unlike
     * {@link LumenTrackedTarget#getBestCameraToTarget()}. Compose with the camera's own mount
     * offset via WPILib's own {@code Pose3d.transformBy(Transform3d cameraToRobot)} to get the
     * robot's field pose - no {@code LumenUtils} helper needed for that one-line composition.
     */
    public Pose3d getFieldToCamera() {
        return fieldToCamera;
    }

    /** How many tags contributed to this solve - always >= 2 (a single tag never produces a
     * multi-tag result; see LumenCamera.getMultiTagResult()'s own note on when this class exists
     * at all vs. an empty Optional). */
    public int getTagCount() {
        return tagCount;
    }

    /** RMS reprojection error in pixels across every contributing tag's corners - a real
     * ambiguity/quality signal (mirroring photonlib's own MultiTargetPNPResult.estimatedPose.
     * ambiguity in spirit): a value climbing well above ~1px on a well-calibrated camera usually
     * means at least one tag's detected corners are noisy (motion blur, a tag near the image
     * edge/corner, partial occlusion) rather than that the solve itself is untrustworthy outright. */
    public double getReprojectionErrorPixels() {
        return reprojectionErrorPixels;
    }
}
