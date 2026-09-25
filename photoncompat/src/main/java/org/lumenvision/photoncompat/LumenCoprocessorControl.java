package org.lumenvision.photoncompat;

import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;

/**
 * REST-based control of a LumenVision coprocessor - driver mode and snapshots (ROADMAP.md Phase
 * 7). For match recording use {@link LumenCoprocessor} instead (NetworkTables, non-blocking). Deliberately a separate class from {@link LumenCamera}, not more methods bolted onto it:
 * LumenCamera is pure NT4 telemetry (low-latency, already-connected, read every loop safely);
 * this is HTTP request/response with real network latency and failure modes (a dropped
 * coprocessor connection, a slow response) - calling these from inside a tight periodic control
 * loop would be a real mistake, the same reason photonlib itself only expects setDriverMode-style
 * calls from occasional, event-driven code (a button press), not every loop iteration.
 *
 * @param baseUrl the coprocessor's own web address, e.g. {@code http://lumenvision.local:5800}
 *     (or the board's IP) - no trailing slash and no {@code /api} suffix; this class adds the
 *     {@code /api} prefix itself.
 */
public class LumenCoprocessorControl {
    private final HttpClient client = HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(2)).build();
    private final String baseUrl;

    public LumenCoprocessorControl(String baseUrl) {
        this.baseUrl = baseUrl;
    }

    // every REST route lives under /api on the coprocessor; building URLs off the bare baseUrl
    // silently 404'd every call before this.
    private URI api(String path) {
        return URI.create(baseUrl + "/api" + path);
    }

    /**
     * Toggles driver mode on a detection sink - it keeps streaming video but stops running
     * detection/publishing results, freeing CPU/NPU time for whatever the driver actually needs
     * the coprocessor for less during that phase of a match.
     *
     * @param sinkId the coprocessor-side sink id (not the source id) - visible via the
     *     coprocessor's own {@code GET /sink/getAll}
     * @return true if the request round-tripped successfully; false on any network/HTTP failure
     *     (logged nowhere by this class - callers decide how to surface that, matching how a
     *     dropped NT4 connection is also silently tolerated by LumenCamera itself)
     */
    public boolean setDriverMode(int sinkId, boolean enabled) {
        return sendPatch("/sink/driverMode?SinkID=" + sinkId + "&Enabled=" + enabled);
    }

    /** Whether driver mode is currently on for the given sink, or false if the request fails
     * (indistinguishable from "off" - callers needing to tell the two apart should catch the
     * underlying exception via a lower-level HTTP call instead). */
    public boolean getDriverMode(int sinkId) {
        try {
            HttpRequest request =
                    HttpRequest.newBuilder(api("/sink/driverMode?SinkID=" + sinkId))
                            .GET()
                            .timeout(Duration.ofSeconds(2))
                            .build();
            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
            return response.statusCode() == 200 && Boolean.parseBoolean(response.body().trim());
        } catch (IOException | InterruptedException e) {
            if (e instanceof InterruptedException) Thread.currentThread().interrupt();
            return false;
        }
    }

    /**
     * Switches which pipeline profile is running for a camera source - the coprocessor-side
     * equivalent of PhotonVision's {@code setPipelineIndex}. Tears down whatever detection sink
     * (AprilTag or object detection) is currently bound to that source and rebuilds it from the
     * chosen profile's own settings; any WebRTC preview or NT4 publishing bound to that same
     * source's detection output keeps working across the switch without needing to be re-bound
     * (ROADMAP.md Phase 7 - see the coprocessor's own SourceManager.ActivateProfile).
     *
     * @param sourceId the coprocessor-side camera source id (not a sink id)
     * @param profileIndex a profile index previously returned by creating a profile via the
     *     coprocessor's own {@code POST /source/profiles/apriltag} or {@code
     *     /source/profiles/objectDetection}
     * @return true if the request round-tripped successfully; false on any network/HTTP failure
     */
    public boolean setPipelineIndex(int sourceId, int profileIndex) {
        return sendPatch("/source/profiles/activate?sourceId=" + sourceId + "&index=" + profileIndex);
    }

    /**
     * The currently active pipeline profile index for a camera source, or -1 if none has ever
     * been activated, or on any network/HTTP failure (indistinguishable from "none activated" -
     * callers needing to tell those apart should catch the underlying exception via a
     * lower-level HTTP call instead, matching {@link #getDriverMode(int)}'s own note).
     */
    public int getPipelineIndex(int sourceId) {
        try {
            HttpRequest request =
                    HttpRequest.newBuilder(api("/source/profiles/active?sourceId=" + sourceId))
                            .GET()
                            .timeout(Duration.ofSeconds(2))
                            .build();
            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
            return response.statusCode() == 200 ? Integer.parseInt(response.body().trim()) : -1;
        } catch (IOException | InterruptedException | NumberFormatException e) {
            if (e instanceof InterruptedException) Thread.currentThread().interrupt();
            return -1;
        }
    }

    /**
     * Saves a source's most recently published frame to a file on the coprocessor itself (not
     * transferred to the robot - this is for post-match/pit review, not something a robot
     * program should poll during a match).
     *
     * @param sourceId the coprocessor-side source id
     * @param fileName a bare file name (e.g. {@code "match17-auto.png"}) - the coprocessor
     *     resolves it under its own fixed snapshots directory regardless of what's passed here,
     *     so a path isn't meaningful
     * @return true if the coprocessor reports it saved successfully
     */
    public boolean saveSnapshot(int sourceId, String fileName) {
        return sendPost("/source/snapshot?SourceID=" + sourceId + "&fileName=" + fileName);
    }

    private boolean sendPatch(String path) {
        return sendNoBody(path, "PATCH");
    }

    private boolean sendPost(String path) {
        return sendNoBody(path, "POST");
    }

    private boolean sendNoBody(String path, String method) {
        try {
            HttpRequest request =
                    HttpRequest.newBuilder(api(path))
                            .method(method, HttpRequest.BodyPublishers.noBody())
                            .timeout(Duration.ofSeconds(2))
                            .build();
            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
            return response.statusCode() >= 200 && response.statusCode() < 300;
        } catch (IOException | InterruptedException e) {
            if (e instanceof InterruptedException) Thread.currentThread().interrupt();
            return false;
        }
    }
}
