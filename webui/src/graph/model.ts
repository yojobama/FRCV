import type { Node, Edge } from '@xyflow/react';
import type { StateSnapshot, WsSinkState, WsSource, WsSink, NodeTypesResponse, NodeTypeCapability } from '../types';

// ROADMAP.md Phase 8c: enum-ordinal -> C# type name tables. Server/Sink.cs's SinkType and
// Server/Source.cs's SourceType are plain enums with no [Description]-style string exposed over
// the wire (WsSink.Type/WsSource.Type are raw ordinals) - these mirror their declaration order
// exactly, matching how hooks/useAppData.ts's mapSinkType already has to.
const SINK_TYPE_NAMES = [
  'ApriltagSink', 'ObjectDetectionSink', 'RecordingSink', 'CameraCalibrationSink',
  'NetworkTablesSink', 'WebRTCSink', 'StereoCalibrationSink', 'StereoDepthSink', 'DepthFusionSink',
];
const SOURCE_TYPE_NAMES = ['Camera', 'ImageFile', 'VideoFile', 'SinkOutput'];

// Terminal/preview sinks render as a badge on the node they're bound to, not their own box - the
// plan's original "toggles on a node's output" decision (already reflected in useAppData.ts's
// PUBLISHABLE_SINK_TYPES on the flip side of the same rule).
const BADGE_SINK_TYPES = new Set(['WebRTCSink', 'NetworkTablesSink']);

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
