package org.lumenvision.photoncompat;

import edu.wpi.first.networktables.DoubleArraySubscriber;
import edu.wpi.first.networktables.DoubleArrayTopic;
import edu.wpi.first.networktables.DoubleSubscriber;
import edu.wpi.first.networktables.NetworkTable;
import edu.wpi.first.networktables.NetworkTableInstance;
import edu.wpi.first.networktables.StringSubscriber;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

/**
 * Robot-side client for one LumenVision coprocessor node, mirroring photonlib's PhotonCamera
 * shape (same idea: construct with a name, poll getLatestResult()) so a team migrating from
 * PhotonVision changes an import line, not their whole vision-handling code.
 *
 * <p>Reads the NT4 schema NetworkTablesSink.cpp publishes under {@code
 * <rootTable>/<sourceName>/tags/*}: parallel DoubleArray topics {@code ids}, {@code x}, {@code
 * y}, {@code z}, and the tag's row-major 3x3 rotation matrix flattened into {@code r0}..{@code
 * r8} - see NetworkTablesSink.cpp and LumenTrackedTarget for why the raw matrix, not a derived
 * Euler/quaternion representation, crosses NT4. Also reads the sibling {@code
 * <rootTable>/<sourceName>/multitag/*} scalars (ROADMAP.md Phase 7) - see
 * {@link #getMultiTagResult()}.
 */
public class LumenCamera {
    private final DoubleArraySubscriber idsSub;
    private final DoubleArraySubscriber xSub;
    private final DoubleArraySubscriber ySub;
    private final DoubleArraySubscriber zSub;
    private final DoubleArraySubscriber[] rSub = new DoubleArraySubscriber[9];

    private final DoubleSubscriber multiTagXSub;
    private final DoubleSubscriber multiTagYSub;
    private final DoubleSubscriber multiTagZSub;
    private final DoubleSubscriber[] multiTagRSub = new DoubleSubscriber[9];
    private final DoubleSubscriber multiTagTagCountSub;
    private final DoubleSubscriber multiTagReprojErrSub;

    private final StringSubscriber versionSub;
    // checked at most once - a coprocessor's ".version" never changes for the lifetime of its
    // process (LumenCore/CMakeLists.txt bakes it in at build time), so there's nothing to gain
    // from re-warning on every single getLatestResult() call once a real mismatch has already
    // been reported.
    private boolean versionChecked = false;

    /**
     * @param instance the NetworkTableInstance to read from - an explicit parameter (not always
     *     {@link NetworkTableInstance#getDefault()}) so a robot program's simulation code can
     *     inject a separate instance instead of this class silently only ever working against
     *     the real robot's default one.
     * @param rootTable must match the coprocessor's own NetworkTablesConfig.rootTable exactly
     *     (the deployment default is "lumenvision" - see the Naming table in ROADMAP.md).
     * @param sourceName must match the bound source's own ID on the coprocessor - the same
     *     string that appears as a subtable under {@code rootTable} in the NT4 tree.
     */
    public LumenCamera(NetworkTableInstance instance, String rootTable, String sourceName) {
        NetworkTable sourceTable = instance.getTable(rootTable + "/" + sourceName);
        NetworkTable tagsTable = sourceTable.getSubTable("tags");
        NetworkTable multiTagTable = sourceTable.getSubTable("multitag");

        idsSub = subscribeArray(tagsTable, "ids");
        xSub = subscribeArray(tagsTable, "x");
        ySub = subscribeArray(tagsTable, "y");
        zSub = subscribeArray(tagsTable, "z");
        for (int i = 0; i < 9; i++) {
            rSub[i] = subscribeArray(tagsTable, "r" + i);
        }

        multiTagXSub = subscribeScalar(multiTagTable, "x");
        multiTagYSub = subscribeScalar(multiTagTable, "y");
        multiTagZSub = subscribeScalar(multiTagTable, "z");
        for (int i = 0; i < 9; i++) {
            multiTagRSub[i] = subscribeScalar(multiTagTable, "r" + i);
        }
        multiTagTagCountSub = subscribeScalar(multiTagTable, "tagCount");
        multiTagReprojErrSub = subscribeScalar(multiTagTable, "reprojErrPixels");

        versionSub = instance.getTable(rootTable).getStringTopic(".version").subscribe("");
    }

    private static DoubleArraySubscriber subscribeArray(NetworkTable table, String name) {
        DoubleArrayTopic topic = table.getDoubleArrayTopic(name);
        return topic.subscribe(new double[0]);
    }

    private static DoubleSubscriber subscribeScalar(NetworkTable table, String name) {
        return table.getDoubleTopic(name).subscribe(0.0);
    }

    /**
     * The most recent detection set published under this camera's table, decoded into WPILib
     * geometry types. Never null - an empty {@link LumenPipelineResult#getTargets()} means "no
     * tags this frame" (including "no frame has ever arrived yet"), not "no data available".
     */
    public LumenPipelineResult getLatestResult() {
        // deferred to here rather than the constructor: right after construction, NT4's own
        // client/server handshake hasn't necessarily completed yet, so ".version" would almost
        // always still read back empty (LumenVersionCheck's own sentinel for "not received yet")
        // and never actually catch a real mismatch - by the time any real robot loop calls
        // getLatestResult() for the first time, the connection has normally long since settled.
        if (!versionChecked) {
            String coprocessorVersion = versionSub.get();
            if (!coprocessorVersion.isEmpty()) {
                LumenVersionCheck.warnOnMismatch(coprocessorVersion);
                versionChecked = true;
            }
        }

        double[] ids = idsSub.get();
        double[] x = xSub.get();
        double[] y = ySub.get();
        double[] z = zSub.get();
        double[][] r = new double[9][];
        for (int i = 0; i < 9; i++) {
            r[i] = rSub[i].get();
        }

        List<LumenTrackedTarget> targets = new ArrayList<>(ids.length);
        for (int i = 0; i < ids.length; i++) {
            // defends against a torn read (one topic updated, another not yet) rather than
            // indexing out of bounds into a shorter array - the parallel arrays are published
            // together in NetworkTablesSink::PublishSourceResult, but NT4 delivers each topic's
            // update independently, so a reader can genuinely observe them out of step for one
            // poll.
            if (i >= x.length || i >= y.length || i >= z.length) break;
            double[] rotationRowMajor = new double[9];
            boolean rotationComplete = true;
            for (int j = 0; j < 9; j++) {
                if (i >= r[j].length) {
                    rotationComplete = false;
                    break;
                }
                rotationRowMajor[j] = r[j][i];
            }
            if (!rotationComplete) break;

            targets.add(new LumenTrackedTarget((int) ids[i], x[i], y[i], z[i], rotationRowMajor));
        }

        // NT4's own subscriber timestamp is already reconciled into this client's local clock
        // domain (that reconciliation is the actual point of NT4 over NT3 - a raw value read out
        // of the JSON/NT payload itself would instead be in the COPROCESSOR's clock, not
        // comparable to Timer.getFPGATimestamp() without doing that reconciliation by hand).
        // Microseconds -> seconds to match what addVisionMeasurement expects.
        double timestampSeconds = idsSub.getLastChange() / 1_000_000.0;

        // read as part of the SAME snapshot as targets/timestamp, not left to a caller's own
        // separate getMultiTagResult() call - LumenPoseEstimator.update(LumenPipelineResult)
        // needs both from one coherent read, not two independent NT4 polls that could
        // legitimately observe two different frames.
        return new LumenPipelineResult(targets, timestampSeconds, getMultiTagResult());
    }

    /**
     * The coprocessor's own multi-tag PnP result (ROADMAP.md Phase 7), if one has been published
     * this frame. Empty when fewer than 2 simultaneously-visible tags have known field poses (no
     * field layout loaded on the coprocessor sink, or fewer than 2 of the tags currently in
     * frame are in it) - {@code multitag/tagCount} is the coprocessor's own explicit signal for
     * this (NetworkTablesSink.cpp publishes 0 there specifically so this doesn't have to guess
     * "stale data" from an unpublished topic apart from "no result this frame").
     */
    public Optional<LumenMultiTagResult> getMultiTagResult() {
        int tagCount = (int) multiTagTagCountSub.get();
        if (tagCount < 2) return Optional.empty();

        double[] rotationRowMajor = new double[9];
        for (int i = 0; i < 9; i++) {
            rotationRowMajor[i] = multiTagRSub[i].get();
        }

        return Optional.of(new LumenMultiTagResult(
                multiTagXSub.get(), multiTagYSub.get(), multiTagZSub.get(),
                rotationRowMajor, tagCount, multiTagReprojErrSub.get()));
    }
}
