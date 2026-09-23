import type { Node, Edge } from '@xyflow/react';
import type { StateSnapshot, WsSinkState, WsSource, WsSink, NodeTypesResponse, NodeTypeCapability } from '../types';

// ROADMAP.md Phase 8c: enum-ordinal -> C# type name tables. Server/Sink.cs's SinkType and
// Server/Source.cs's SourceType are plain enums with no [Description]-style string exposed over
// the wire (WsSink.Type/WsSource.Type are raw ordinals) - these mirror their declaration order
// exactly, matching how hooks/useAppData.ts's mapSinkType already has to.
// index 2 stays a reserved gap, not 'CameraCalibrationSink' shifted down - Server/Sink.cs's
// SinkType enum pins explicit numeric values for exactly this reason (2 was RecordingSink,
// deleted but never reused, since it's a raw ordinal over the wire with no string converter).
const SINK_TYPE_NAMES = [
  'ApriltagSink', 'ObjectDetectionSink', undefined, 'CameraCalibrationSink',
  'NetworkTablesSink', 'WebRTCSink', 'StereoCalibrationSink', 'StereoDepthSink', 'DepthFusionSink',
  'MjpegSink', 'RecordSink',
];
const SOURCE_TYPE_NAMES = ['Camera', 'ImageFile', 'VideoFile', 'SinkOutput'];

// Terminal/preview sinks render as a badge on the node they're bound to, not their own box - the
// plan's original "toggles on a node's output" decision. MjpegSink joins WebRTCSink here for the
// same reason: StreamView.tsx spins one up as a same-preview fallback when WebRTC breaks, and it
// should disappear back into the badge it's standing in for, not get its own graph box.
// RecordSink is the same shape again - terminal, single-source, toggled on a node's output.
const BADGE_SINK_TYPES = new Set(['WebRTCSink', 'NetworkTablesSink', 'MjpegSink', 'RecordSink']);

export function sinkTypeName(ordinal: number): string {
  return SINK_TYPE_NAMES[ordinal] ?? 'Unknown';
}
export function sourceTypeName(ordinal: number): string {
  return SOURCE_TYPE_NAMES[ordinal] ?? 'Unknown';
}

export interface PipelineNodeData extends Record<string, unknown> {
  kind: 'source' | 'sink';
  label: string;
  typeName: string;
  capability: NodeTypeCapability | null;
  fps: number;
  latencyUs: number;
  // the raw server object - the Inspector reads ids/bindings/profiles directly off this rather
  // than this file re-deriving a second, narrower copy of the same data.
  raw: WsSource | WsSink;
  // sink nodes only
  isRunning?: boolean;
  webrtcSink?: WsSinkState;
  nt4Sink?: WsSinkState;
  mjpegSink?: WsSinkState;
  recordSink?: WsSinkState;
  // source nodes only
  activeProfileIndex?: number;
  profileCount?: number;
}

export type PipelineNode = Node<PipelineNodeData, 'pipelineNode'>;

const COLUMN_WIDTH = 260;
const ROW_HEIGHT = 140;

// Positions are kept OUTSIDE this function (by the caller, via PositionStore) - buildGraph is
// called on every /ws/state tick (roughly once a second), and blindly recomputing a layout each
// time would fight a user's own drag with their own data, snapping nodes back mid-drag. A node
// id not yet in the store gets a fresh auto-layout slot; one already there keeps exactly where
// it was left (dragged or auto-placed).
export interface PositionStore {
  get(id: string): { x: number; y: number } | undefined;
  set(id: string, pos: { x: number; y: number }): void;
}

export function buildGraph(
  snapshot: StateSnapshot,
  capabilities: NodeTypesResponse | null,
  positions: PositionStore
): { nodes: PipelineNode[]; edges: Edge[] } {
  const nodes: PipelineNode[] = [];
  const edges: Edge[] = [];
  const sourceColumnCount: Record<string, number> = {};
  let sourceRow = 0;

  const findSourceCap = (ordinal: number) =>
    capabilities?.Sources.find(c => c.TypeName === sourceTypeName(ordinal)) ?? null;
  const findSinkCap = (ordinal: number) =>
    capabilities?.Sinks.find(c => c.TypeName === sinkTypeName(ordinal)) ?? null;

  const placeAt = (id: string, fallbackCol: number, fallbackRow: number) => {
    let pos = positions.get(id);
    if (!pos) {
      pos = { x: fallbackCol * COLUMN_WIDTH, y: fallbackRow * ROW_HEIGHT };
      positions.set(id, pos);
    }
    return pos;
  };

  // Source nodes (column 0)
  for (const source of snapshot.Sources) {
    const id = `source-${source.Id}`;
    const stats = snapshot.NodeStats[String(source.Id)];

    // badges: same dual-role lookup the sink loop below does, just keyed on this SOURCE's own
    // id instead of a sink's output id - a WebRTCSink/NetworkTablesSink/MjpegSink can bind
    // directly to a raw source (e.g. previewing a camera before any detector is attached to it),
    // not only to a sink's own output. Previously only computed in the sink loop, which is why a
    // source node's own "Live Preview" never had a webrtcSink to render against.
    const webrtcSink = snapshot.Sinks.find(s => sinkTypeName(s.Sink.Type) === 'WebRTCSink' && s.Sink.Source?.Id === source.Id);
    const nt4Sink = snapshot.Sinks.find(s => sinkTypeName(s.Sink.Type) === 'NetworkTablesSink' && s.Sink.Source?.Id === source.Id);
    const mjpegSink = snapshot.Sinks.find(s => sinkTypeName(s.Sink.Type) === 'MjpegSink' && s.Sink.Source?.Id === source.Id);
    const recordSink = snapshot.Sinks.find(s => sinkTypeName(s.Sink.Type) === 'RecordSink' && s.Sink.Source?.Id === source.Id);

    nodes.push({
      id,
      type: 'pipelineNode',
      position: placeAt(id, 0, sourceRow),
      data: {
        kind: 'source',
        label: source.Name,
        typeName: sourceTypeName(source.Type),
        capability: findSourceCap(source.Type),
        fps: stats?.Fps ?? 0,
        latencyUs: stats?.LatencyUs ?? 0,
        raw: source,
        webrtcSink,
        nt4Sink,
        mjpegSink,
        recordSink,
        activeProfileIndex: source.ActiveProfileIndex,
        profileCount: source.Profiles.length,
      },
    });
    sourceRow++;
  }

  // Sink nodes (graph-shaped types only - badges attach to whichever node they're bound to
  // instead of getting their own box) and edges for their bindings.
  const graphSinks = snapshot.Sinks.filter(s => {
    const cap = findSinkCap(s.Sink.Type);
    return cap?.Implemented && !BADGE_SINK_TYPES.has(sinkTypeName(s.Sink.Type));
  });

  for (const sinkState of graphSinks) {
    const sink = sinkState.Sink;
    const id = `sink-${sink.Id}`;
    const stats = snapshot.NodeStats[String(sink.Id)];

    // badges: a WebRTCSink/NetworkTablesSink bound to THIS sink's own output (dual-role sinks
    // register themselves as a source too - see SinkManager.DualRoleSinkTypes)
    const webrtcSink = snapshot.Sinks.find(s => sinkTypeName(s.Sink.Type) === 'WebRTCSink' && s.Sink.Source?.Id === sink.Id);
    const nt4Sink = snapshot.Sinks.find(s => sinkTypeName(s.Sink.Type) === 'NetworkTablesSink' && s.Sink.Source?.Id === sink.Id);
    // the StreamView.tsx-created same-preview MJPEG fallback, if WebRTC has broken for this
    // node's preview and one's already been spun up - same dual-role lookup as webrtcSink above.
    const mjpegSink = snapshot.Sinks.find(s => sinkTypeName(s.Sink.Type) === 'MjpegSink' && s.Sink.Source?.Id === sink.Id);
    const recordSink = snapshot.Sinks.find(s => sinkTypeName(s.Sink.Type) === 'RecordSink' && s.Sink.Source?.Id === sink.Id);

    // column: one to the right of whatever this sink's primary (left, for stereo) source is
    // sitting in, so the graph visually flows left-to-right with data - falls back to column 1
    // if unbound yet (a freshly-created, not-yet-connected sink).
    const upstreamId = sink.Source ? `source-${sink.Source.Id}` : null;
    const upstreamPos = upstreamId ? positions.get(upstreamId) : undefined;
    const col = upstreamPos ? Math.round(upstreamPos.x / COLUMN_WIDTH) + 1 : 1;
    const row = sourceColumnCount[col] ?? 0;
    sourceColumnCount[col] = row + 1;

    nodes.push({
      id,
      type: 'pipelineNode',
      position: placeAt(id, col, row),
      data: {
        kind: 'sink',
        label: sink.Name,
        typeName: sinkTypeName(sink.Type),
        capability: findSinkCap(sink.Type),
        fps: stats?.Fps ?? 0,
        latencyUs: stats?.LatencyUs ?? 0,
        raw: sink,
        isRunning: sinkState.IsRunning,
        webrtcSink,
        nt4Sink,
        mjpegSink,
        recordSink,
      },
    });

    if (sink.Source && sink.Source2) {
      // stereo sink - Source is LEFT, Source2 is RIGHT (Server/Sink.cs's own Source2 comment)
      edges.push({ id: `${id}-left`, source: `source-${sink.Source.Id}`, target: id, label: 'left', type: 'smoothstep' });
      edges.push({ id: `${id}-right`, source: `source-${sink.Source2.Id}`, target: id, label: 'right', type: 'smoothstep' });
    } else if (sink.Source) {
      edges.push({ id, source: `source-${sink.Source.Id}`, target: id, type: 'smoothstep' });
    }

    if (sink.DepthSourceId != null) {
      // DepthFusionSink's depth-grid attach - not an ordinary Source bind (AttachDepthFusionSource
      // reads a sibling StereoDepthSink's output directly, see Sink.cs's own DepthSourceId comment)
      edges.push({ id: `${id}-depth`, source: `sink-${sink.DepthSourceId}`, target: id, label: 'depth', type: 'smoothstep' });
    }
  }

  return { nodes, edges };
}

// buildGraph builds a brand new `edges` array (with brand new edge objects) every call, but
// edges only actually change when a binding is made/broken - which is rare compared to how often
// buildGraph runs (once per /ws/state tick, ~1/sec). Passing a new array+objects to React Flow's
// `edges` prop every tick regardless forces it to redo edge-path layout (and the MiniMap to
// redraw) every second even when literally nothing rebound. The caller should keep its previous
// edges array and call this before calling setEdges - reuse the OLD array reference when it
// returns true, so React Flow's own prop-identity check skips that work entirely.
export function edgesEqual(a: Edge[], b: Edge[]): boolean {
  if (a.length !== b.length) return false;
  for (let i = 0; i < a.length; i++) {
    if (a[i].id !== b[i].id || a[i].source !== b[i].source || a[i].target !== b[i].target || a[i].label !== b[i].label) {
      return false;
    }
  }
  return true;
}
