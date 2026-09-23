package org.lumenvision.photoncompat.compat;

import org.lumenvision.photoncompat.LumenPipelineResult;

import java.util.List;
import java.util.stream.Collectors;

/** Thin {@link LumenPipelineResult} wrapper - see {@link PhotonCamera}'s own class comment for
 * why this package exists. */
public class PhotonPipelineResult {
    private final LumenPipelineResult lumenResult;

    PhotonPipelineResult(LumenPipelineResult lumenResult) {
        this.lumenResult = lumenResult;
    }

    public List<PhotonTrackedTarget> getTargets() {
        return lumenResult.getTargets().stream()
                .map(PhotonTrackedTarget::new)
                .collect(Collectors.toList());
    }

    public boolean hasTargets() {
        return lumenResult.hasTargets();
    }

    public double getTimestampSeconds() {
        return lumenResult.getTimestampSeconds();
    }

    public LumenPipelineResult getLumenPipelineResult() {
        return lumenResult;
    }
}
