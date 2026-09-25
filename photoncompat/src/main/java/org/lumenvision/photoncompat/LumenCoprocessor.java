package org.lumenvision.photoncompat;

import edu.wpi.first.networktables.BooleanPublisher;
import edu.wpi.first.networktables.BooleanSubscriber;
import edu.wpi.first.networktables.NetworkTable;
import edu.wpi.first.networktables.NetworkTableInstance;

/**
 * Coprocessor-wide controls over NetworkTables - currently match recording.
 *
 * <p>Unlike {@link LumenCoprocessorControl} (HTTP, blocks the calling thread for up to its 2 s
 * timeout when the coprocessor is unreachable), every call here is a non-blocking NT4 publish or
 * a cached subscriber read, so it's safe to call from periodic robot code, e.g.
 * {@code autonomousInit()} / {@code disabledInit()}:
 *
 * <pre>{@code
 * LumenCoprocessor coprocessor = new LumenCoprocessor(NetworkTableInstance.getDefault());
 *
 * public void autonomousInit() { coprocessor.startRecording(); }
 * public void disabledInit()   { coprocessor.stopRecording(); }
 * }</pre>
 *
 * <p>Recording is desired state, not a toggle: the robot publishes {@code
 * <rootTable>/config/recording}, and while it's true the coprocessor records every camera (the
 * same thing the web UI's Match View "Start Recording" button does - one RecordSink per camera,
 * segmented MP4 plus a telemetry sidecar). If the coprocessor reboots mid-match it picks the
 * retained value back up and resumes. {@link #isRecording()} reads {@code
 * <rootTable>/status/recording}, which the coprocessor publishes, so robot code can confirm the
 * request actually took effect (e.g. show it on the dashboard).
 *
 * <p>Requires the coprocessor to have a NetworkTables sink connected to this robot - which it
 * already needs for the robot to receive any vision results.
 */
public class LumenCoprocessor {
    /** The coprocessor's default NT root table (NetworkTablesConfig.rootTable). */
    public static final String DEFAULT_ROOT_TABLE = "lumenvision";

    private final BooleanPublisher recordingRequestPub;
    private final BooleanSubscriber recordingStatusSub;

    /** Uses the coprocessor's default root table, {@value #DEFAULT_ROOT_TABLE}. */
    public LumenCoprocessor(NetworkTableInstance instance) {
        this(instance, DEFAULT_ROOT_TABLE);
    }

    /**
     * @param instance the NetworkTableInstance to use - explicit (not always the default one) so
     *     simulation code can inject its own, same as {@link LumenCamera}
     * @param rootTable must match the coprocessor's NetworkTablesConfig.rootTable exactly
     */
    public LumenCoprocessor(NetworkTableInstance instance, String rootTable) {
        NetworkTable root = instance.getTable(rootTable);
        recordingRequestPub = root.getSubTable("config").getBooleanTopic("recording").publish();
        recordingStatusSub = root.getSubTable("status").getBooleanTopic("recording").subscribe(false);
    }

    /** Asks the coprocessor to start recording every camera. */
    public void startRecording() {
        setRecording(true);
    }

    /** Asks the coprocessor to stop recording (the current segment is finalized, not lost). */
    public void stopRecording() {
        setRecording(false);
    }

    /** Sets the desired recording state - see the class comment for the semantics. */
    public void setRecording(boolean recording) {
        recordingRequestPub.set(recording);
    }

    /**
     * Whether the coprocessor reports it's actually recording right now. False until the
     * coprocessor has published its status at least once (e.g. not connected yet).
     */
    public boolean isRecording() {
        return recordingStatusSub.get();
    }
}
