package org.lumenvision.photoncompat.compat;

import edu.wpi.first.networktables.NetworkTableInstance;
import org.lumenvision.photoncompat.LumenCamera;

/**
 * Thin wrapper giving {@link LumenCamera} PhotonVision's own class name/shape, so a team
 * migrating from PhotonVision to LumenVision changes only its {@code import} lines - every
 * method here does nothing but delegate to the real {@link LumenCamera} instance underneath.
 * Prefer {@code org.lumenvision.photoncompat.LumenCamera} directly in new code; this package
 * exists purely for drop-in migration.
 */
public class PhotonCamera {
    private final LumenCamera lumenCamera;

    /**
     * Matches {@code PhotonCamera(String cameraName)}'s own convenience shape: the default NT4
     * instance and this project's own default root table ("lumenvision" - see
     * NetworkTablesConfig::rootTable's default in NetworkTablesSink.h).
     */
    public PhotonCamera(String cameraName) {
        this(NetworkTableInstance.getDefault(), "lumenvision", cameraName);
    }

    public PhotonCamera(NetworkTableInstance instance, String rootTable, String cameraName) {
        this.lumenCamera = new LumenCamera(instance, rootTable, cameraName);
    }

    public PhotonPipelineResult getLatestResult() {
        return new PhotonPipelineResult(lumenCamera.getLatestResult());
    }

    /** Escape hatch to the real underlying client, for anything this thin wrapper doesn't
     * (yet) mirror - e.g. {@code LumenCamera.getMultiTagResult()}, which has no PhotonVision
     * equivalent to name this wrapper after. */
    public LumenCamera getLumenCamera() {
        return lumenCamera;
    }
}
