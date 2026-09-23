import React, { useCallback, useEffect, useRef, useState } from 'react';
import {
  ReactFlow, ReactFlowProvider, Background, Controls, MiniMap,
  useNodesState, type Edge, type Connection, type IsValidConnection,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';
import { Plus, Camera as CameraIcon, Target } from 'lucide-react';
import { useStateSocket } from '../hooks/useStateSocket';
import { ApiService } from '../services/ApiService';
import { buildGraph, edgesEqual, type PipelineNode, type PositionStore } from '../graph/model';
import { nodeTypes } from '../graph/PipelineNode';
import { Inspector } from '../graph/Inspector';
import { GraphProfileBar } from '../graph/GraphProfileBar';
import { LeftRail } from '../graph/LeftRail';
import { BottomStrip } from '../graph/BottomStrip';
import { AddSourceModal } from '../components/AddSourceModal';
import { AddSinkModal } from '../components/AddSinkModal';
import type { AddSinkOptions, NodeTypesResponse, NT4Defaults, CameraHardwareInfo } from '../types';

const api = new ApiService();

// ROADMAP.md Phase 8c: the pipeline graph - the core of Phase 8. Sources and the graph-shaped
// sink types are canvas nodes (see graph/model.ts for exactly which - terminal/preview sinks
// render as badges instead); edges are the real bindings, driven live off /ws/state rather than
// polled. Node creation stays modal-based for this pass (reusing AddSourceModal/AddSinkModal's
// existing form logic) but the created node lands on the canvas instead of a list, and dragging
// a connection between two nodes performs the real bind through the same REST endpoints the old
// Configure modals used - drag/drop replaces the dropdown-based binding UI, not the API under it.
const GraphPageInner: React.FC<{ onToast: (m: string, t: 'success'|'error'|'info') => void; nt4Settings: NT4Defaults; darkMode: boolean }> = ({ onToast, nt4Settings, darkMode }) => {
  const { snapshot, connected } = useStateSocket();
  const [capabilities, setCapabilities] = useState<NodeTypesResponse | null>(null);
  const [nodes, setNodes, onNodesChange] = useNodesState<PipelineNode>([]);
  const [edges, setEdges] = useState<Edge[]>([]);
  const [selectedId, setSelectedId] = useState<string | null>(null);
  const [showAddSource, setShowAddSource] = useState(false);
  const [showAddSink, setShowAddSink] = useState(false);
  const positionsRef = useRef(new Map<string, { x: number; y: number }>());

  useEffect(() => {
    api.getNodeTypeCapabilities().then(setCapabilities).catch(() => onToast('Failed to load node capabilities', 'error'));
  }, []);

  // keep the position store in sync with whatever React Flow's own drag handling has already
  // done to the live node array, so the next snapshot-driven rebuild below picks up drags
  // instead of fighting them.
  useEffect(() => {
    for (const n of nodes) positionsRef.current.set(n.id, n.position);
  }, [nodes]);

  useEffect(() => {
    if (!snapshot) return;
    const store: PositionStore = {
      get: (id) => positionsRef.current.get(id),
      set: (id, pos) => positionsRef.current.set(id, pos),
    };
    const built = buildGraph(snapshot, capabilities, store);
    setNodes(built.nodes);
    // reuse the previous array reference when the edge set hasn't actually changed - see
    // edgesEqual's own comment for why this matters every tick, not just as a micro-opt.
    setEdges(prev => (edgesEqual(prev, built.edges) ? prev : built.edges));
  }, [snapshot, capabilities, setNodes]);

  const selectedNode = nodes.find(n => n.id === selectedId) ?? null;
  // WebRTCSink only ever holds ONE active peer connection (InitializePeerConnection replaces it
  // wholesale on every new negotiation - see WebRTCSink.h's own comment) - if Inspector's own
  // Live Preview and BottomStrip's "thumbnail for every running WebRTC sink" both try to
  // negotiate against the SAME sink at once, one negotiation's answer arrives after the sink has
  // already gone 'stable' from the other and gets rejected outright (confirmed the hard way:
  // WebRTCSink::SetAnswer failed - "Unexpected remote answer description in signaling state
  // stable" - which StreamView.tsx then (correctly, but pointlessly) treats as a real failure and
  // falls back to MJPEG). Excluding whichever sink Inspector is already showing from BottomStrip
  // avoids the double-negotiation instead of racing them.
  const inspectorPreviewSinkId = selectedNode?.data.webrtcSink?.IsRunning ? selectedNode.data.webrtcSink.Sink.Id : null;

  const onConnect = useCallback(async (connection: Connection) => {
    // source-{id} -> sink-{id}: bind. sink-{id} -> sink-{id} (a StereoDepthSink's own output
    // feeding a DepthFusionSink): attach as the depth source. Anything else isn't meaningful.
    const [sourceKind, sourceRawId] = connection.source.split('-');
    const [targetKind, targetRawId] = connection.target.split('-');
    if (targetKind !== 'sink') return;
    const targetSinkId = Number(targetRawId);

    try {
      if (sourceKind === 'source') {
        const targetNode = nodes.find(n => n.id === connection.target);
        if (targetNode?.data.capability?.SourceRoles) {
          // stereo sink - ask which role this connection fills rather than guessing
          const role = prompt(`Bind as "left" or "right" camera?`, 'left');
          if (role !== 'left' && role !== 'right') return;
          // the bind call needs both roles' ids at once - reuse whichever role is already bound
          // (if any) as the other side.
          const existingEdge = edges.find(e => e.target === connection.target);
          const existingSourceId = existingEdge ? Number(existingEdge.source.split('-')[1]) : undefined;
          const leftId = role === 'left' ? Number(sourceRawId) : existingSourceId;
          const rightId = role === 'right' ? Number(sourceRawId) : existingSourceId;
          if (leftId == null || rightId == null) {
            onToast('Bind the other camera role first, or use a single-source sink', 'error');
            return;
          }
          if (targetNode.data.typeName === 'StereoDepthSink') {
            await api.bindStereoDepthSources(targetSinkId, leftId, rightId);
          } else {
            await api.bindStereoSources(targetSinkId, leftId, rightId);
          }
        } else {
          await api.bindSinkToSource(targetSinkId, Number(sourceRawId));
        }
        onToast('Connected', 'success');
      } else if (sourceKind === 'sink') {
        // depth attach: only meaningful source-sink is a StereoDepthSink feeding a
        // DepthFusionSink's HasDepthAttach input.
        await api.attachDepthFusionSource(targetSinkId, Number(sourceRawId));
        onToast('Depth source attached', 'success');
      }
    } catch {
      onToast('Failed to connect - check the connection is valid for these node types', 'error');
    }
  }, [nodes, edges, onToast]);

  const isValidConnection: IsValidConnection = useCallback((edgeOrConn) => {
    const sourceId = 'source' in edgeOrConn ? edgeOrConn.source : undefined;
    const targetId = 'target' in edgeOrConn ? edgeOrConn.target : undefined;
    if (!sourceId || !targetId || sourceId === targetId) return false;
    const targetNode = nodes.find(n => n.id === targetId);
    const sourceNode = nodes.find(n => n.id === sourceId);
    if (!targetNode || !sourceNode || targetNode.data.kind !== 'sink') return false;

    const cap = targetNode.data.capability;
    if (!cap) return false;

    if (cap.HasDepthAttach) {
      // DepthFusionSink: its depth input only accepts a StereoDepthSink's own output
      return sourceNode.data.kind === 'sink' && sourceNode.data.typeName === 'StereoDepthSink';
    }

    // ordinary/stereo bind: source must be a real Source or a dual-role sink acting as one
    if (sourceNode.data.kind === 'sink' && !sourceNode.data.capability?.IsDualRoleSink) return false;

    const existingCount = edges.filter(e => e.target === targetId).length;
    return existingCount < cap.MaxSources;
  }, [nodes, edges]);

  const placeNewNode = () => {
    // simple staggered placement for newly-created nodes not yet reported by the server (the
    // next snapshot tick gives them a real id-keyed slot in the position store)
    const count = nodes.length;
    return { x: (count % 4) * 260, y: Math.floor(count / 4) * 140 + 400 };
  };

  const handleAddSource = async (name: string, type: string, files?: FileList, fps?: number, hardwareInfo?: CameraHardwareInfo) => {
    try {
      if (type === 'camera' && hardwareInfo) {
        await api.createCameraSource(hardwareInfo, name);
      } else if (type === 'video' && files?.length) {
        const result = await api.uploadVideoFiles(files, fps ?? 30);
        if (!result.success) { onToast(result.message, 'error'); return; }
      } else if (type === 'image' && files?.length) {
        const result = await api.uploadImageFiles(files);
        if (!result.success) { onToast(result.message, 'error'); return; }
      }
      onToast(`Source "${name}" added`, 'success');
      setShowAddSource(false);
    } catch {
      onToast('Failed to add source', 'error');
    }
  };

  const handleAddSink = async (name: string, type: string, options?: AddSinkOptions) => {
    try {
      if (type === 'ApriltagSink') {
        await api.createApriltagSinkWithBackend(name, options?.tagSize ?? 0.1651, options?.backend ?? 0);
      } else if (type === 'calibration') {
        await api.createCameraCalibrationSink(name);
      } else if (type === 'object' && options?.modelId) {
        await api.createObjectDetectionSink(name, options.modelId);
      }
      onToast(`Sink "${name}" added`, 'success');
      setShowAddSink(false);
    } catch {
      onToast('Failed to add sink', 'error');
    }
  };

  return (
    <div className="flex h-[calc(100vh-140px)] -m-6">
      <LeftRail snapshot={snapshot} onToast={onToast} />
      {/* flex column, not another absolute overlay: BottomStrip used to be positioned
          absolute/bottom-0 directly on top of the canvas, covering React Flow's own
          bottom-anchored Controls/MiniMap panels entirely (they render within the ReactFlow
          container's own bounds, so there was no way to reach them underneath it). Giving
          BottomStrip its natural (dynamic - 1 or 2 rows depending on how many WebRTC sinks are
          live) height in normal flow instead, with the canvas taking the remaining space above
          it, means the two can never overlap regardless of BottomStrip's height. */}
      <div className="flex-1 flex flex-col min-h-0">
        <div className="flex-1 relative min-h-0">
          <div className="absolute top-4 left-4 z-10 flex gap-2">
            <button onClick={() => setShowAddSource(true)} className="px-3 py-2 bg-blue-600 text-white rounded shadow hover:bg-blue-700 flex items-center gap-2 text-sm">
              <CameraIcon className="w-4 h-4" /><Plus className="w-3 h-3" />Source
            </button>
            <button onClick={() => setShowAddSink(true)} className="px-3 py-2 bg-green-600 text-white rounded shadow hover:bg-green-700 flex items-center gap-2 text-sm">
              <Target className="w-4 h-4" /><Plus className="w-3 h-3" />Sink
            </button>
            {!connected && (
              <span className="px-3 py-2 bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200 rounded text-sm">Reconnecting to live state...</span>
            )}
            <GraphProfileBar onToast={onToast} />
          </div>
          <ReactFlow
            nodes={nodes}
            edges={edges}
            nodeTypes={nodeTypes}
            onNodesChange={onNodesChange}
            onConnect={onConnect}
            isValidConnection={isValidConnection}
            onNodeClick={(_, node) => setSelectedId(node.id)}
            onPaneClick={() => setSelectedId(null)}
            colorMode={darkMode ? 'dark' : 'light'}
            fitView
          >
            <Background />
            <Controls />
            <MiniMap />
          </ReactFlow>
        </div>
        <BottomStrip snapshot={snapshot} excludeSinkId={inspectorPreviewSinkId} />
      </div>

      {selectedNode && (
        <Inspector
          node={selectedNode}
          onClose={() => setSelectedId(null)}
          onToast={onToast}
          onDeleted={() => setSelectedId(null)}
          nt4Settings={nt4Settings}
        />
      )}

      <AddSourceModal isOpen={showAddSource} onClose={() => setShowAddSource(false)} onAdd={handleAddSource} />
      <AddSinkModal isOpen={showAddSink} onClose={() => setShowAddSink(false)} onAdd={handleAddSink} />
    </div>
  );
};

export const GraphPage: React.FC<{ onToast: (m: string, t: 'success'|'error'|'info') => void; nt4Settings: NT4Defaults; darkMode: boolean }> = (props) => (
  <ReactFlowProvider>
    <GraphPageInner {...props} />
  </ReactFlowProvider>
);
