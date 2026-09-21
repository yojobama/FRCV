package org.lumenvision.photoncompat;

import edu.wpi.first.networktables.DoubleArraySubscriber;
import edu.wpi.first.networktables.DoubleArrayTopic;
import edu.wpi.first.networktables.NetworkTable;
import edu.wpi.first.networktables.NetworkTableInstance;

import java.util.ArrayList;
import java.util.List;

/**
 * Robot-side client for one LumenVision coprocessor node, mirroring photonlib's PhotonCamera
 * shape (same idea: construct with a name, poll getLatestResult()) so a team migrating from
 * PhotonVision changes an import line, not their whole vision-handling code.
 *
 * <p>Reads the NT4 schema NetworkTablesSink.cpp publishes under {@code
 * <rootTable>/<sourceName>/tags/*}: parallel DoubleArray topics {@code ids}, {@code x}, {@code
 * y}, {@code z}, and the tag's row-major 3x3 rotation matrix flattened into {@code r0}..{@code
 * r8} - see NetworkTablesSink.cpp and LumenTrackedTarget for why the raw matrix, not a derived
 * Euler/quaternion representation, crosses NT4.
 */
public class LumenCamera {
    private final DoubleArraySubscriber idsSub;
    private final DoubleArraySubscriber xSub;
    private final DoubleArraySubscriber ySub;
    private final DoubleArraySubscriber zSub;
    private final DoubleArraySubscriber[] rSub = new DoubleArraySubscriber[9];

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
        NetworkTable table = instance.getTable(rootTable + "/" + sourceName + "/tags");

        idsSub = subscribe(table, "ids");
        xSub = subscribe(table, "x");
        ySub = subscribe(table, "y");
        zSub = subscribe(table, "z");
        for (int i = 0; i < 9; i++) {
            rSub[i] = subscribe(table, "r" + i);
        }
    }

    private static DoubleArraySubscriber subscribe(NetworkTable table, String name) {
        DoubleArrayTopic topic = table.getDoubleArrayTopic(name);
        return topic.subscribe(new double[0]);
    }

    /**
     * The most recent detection set published under this camera's table, decoded into WPILib
     * geometry types. Never null - an empty {@link LumenPipelineResult#getTargets()} means "no
     * tags this frame" (including "no frame has ever arrived yet"), not "no data available".
     */
    public LumenPipelineResult getLatestResult() {
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

        return new LumenPipelineResult(targets, timestampSeconds);
    }
}
