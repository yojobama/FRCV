import React from 'react';
import { Handle, Position, type NodeProps } from '@xyflow/react';
import {
  Camera, Video, Image as ImageIcon, Scan, Box, Grid3x3, Radio, Layers, Combine, HelpCircle,
  Wifi, WifiOff, Radio as RadioIcon,
} from 'lucide-react';
import type { PipelineNode as PipelineNodeType } from './model';

const ICONS: Record<string, React.ComponentType<{ className?: string }>> = {
  camera: Camera,
  video: Video,
  image: ImageIcon,
  scan: Scan,
  box: Box,
  grid: Grid3x3,
  radio: Radio,
  layers: Layers,
  combine: Combine,
};

// ROADMAP.md Phase 8c: one generic node renderer for both sources and the graph-shaped sink
// types, coloured by class (source = blue, sink = green) per the plan's own "nodes coloured by
// class" wording. Every node gets both handles - which edges are actually valid between them is
// enforced by isValidConnection in GraphPage, not by which handles exist, since a dual-role sink
// (a StereoDepthSink feeding a DepthFusionSink's depth input) is legitimately both a target and
// a source at once.
const PipelineNodeImpl: React.FC<NodeProps<PipelineNodeType>> = ({ data, selected }) => {
  const Icon = ICONS[data.capability?.Icon ?? ''] ?? HelpCircle;
  const isSource = data.kind === 'source';

  return (
    <div
      className={`rounded-lg shadow-lg border-2 px-4 py-3 min-w-[200px] bg-white dark:bg-gray-800 ${
        selected ? 'border-blue-500' : isSource ? 'border-blue-300 dark:border-blue-700' : 'border-green-300 dark:border-green-700'
      }`}
    >
      <Handle type="target" position={Position.Left} id="in" className="!w-3 !h-3" />
      <Handle type="source" position={Position.Right} id="out" className="!w-3 !h-3" />

      <div className="flex items-center gap-2 mb-1">
        <Icon className={`w-4 h-4 ${isSource ? 'text-blue-600' : 'text-green-600'}`} />
        <span className="font-semibold text-sm text-gray-900 dark:text-white truncate">{data.label}</span>
      </div>
      <div className="text-xs text-gray-500 dark:text-gray-400 mb-2">{data.capability?.DisplayName ?? data.typeName}</div>

      <div className="flex items-center gap-3 text-xs text-gray-600 dark:text-gray-300">
        <span title="Frames per second">{data.fps.toFixed(1)} fps</span>
        <span title="Latency">{(data.latencyUs / 1000).toFixed(1)} ms</span>
        {data.kind === 'sink' && (
          <span className={`px-1.5 py-0.5 rounded text-xs font-medium ${data.isRunning ? 'bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200' : 'bg-gray-100 text-gray-600 dark:bg-gray-700 dark:text-gray-300'}`}>
            {data.isRunning ? 'running' : 'stopped'}
          </span>
        )}
      </div>

      {isSource && (data.profileCount ?? 0) > 0 && (
        <div className="mt-1 text-xs text-purple-600 dark:text-purple-400">
          Profile {data.activeProfileIndex! >= 0 ? data.activeProfileIndex : '(none active)'} / {data.profileCount}
        </div>
      )}

      {(data.webrtcSink || data.nt4Sink) && (
        <div className="flex items-center gap-2 mt-2 pt-2 border-t border-gray-100 dark:border-gray-700">
          {data.webrtcSink && (
            <span className={`flex items-center gap-1 text-xs px-1.5 py-0.5 rounded ${data.webrtcSink.IsRunning ? 'bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200' : 'bg-gray-100 text-gray-500 dark:bg-gray-700 dark:text-gray-400'}`} title="WebRTC preview">
              {data.webrtcSink.IsRunning ? <Wifi className="w-3 h-3" /> : <WifiOff className="w-3 h-3" />}Preview
            </span>
          )}
          {data.nt4Sink && (
            <span className={`flex items-center gap-1 text-xs px-1.5 py-0.5 rounded ${data.nt4Sink.IsRunning ? 'bg-blue-100 text-blue-800 dark:bg-blue-900 dark:text-blue-200' : 'bg-gray-100 text-gray-500 dark:bg-gray-700 dark:text-gray-400'}`} title="NetworkTables publish">
              <RadioIcon className="w-3 h-3" />NT4
            </span>
          )}
        </div>
      )}
    </div>
  );
};

// buildGraph (graph/model.ts) rebuilds the whole nodes array from scratch on every /ws/state
// tick (~1/sec) - every node gets a brand new `data` object every time, even one whose displayed
// values haven't actually changed (an idle/disabled sink, a source sitting at a steady fps).
// Without this, React Flow re-renders every node's full DOM subtree every tick regardless -
// this is what made the graph feel laggy with more than a handful of nodes on screen. The
// comparator checks the same fields PipelineNode actually renders, at the same precision it
// displays them at (fps/latency compared as their rendered strings, not raw floats, so
// imperceptible jitter like 14.98 -> 15.02 - both "15.0" on screen - doesn't force a re-render
// either). `raw` is deliberately NOT compared: PipelineNode never reads it (only Inspector does,
// off the live `nodes` state directly, not off this memoized component), so ignoring it here
// costs nothing and staleness never leaks through to the Inspector.
function pipelineNodePropsEqual(prev: NodeProps<PipelineNodeType>, next: NodeProps<PipelineNodeType>): boolean {
  if (prev.selected !== next.selected) return false;
  const a = prev.data, b = next.data;
  if (a === b) return true;
  return (
    a.kind === b.kind &&
    a.label === b.label &&
    a.typeName === b.typeName &&
    a.capability === b.capability &&
    a.fps.toFixed(1) === b.fps.toFixed(1) &&
    a.latencyUs.toFixed(0) === b.latencyUs.toFixed(0) &&
    a.isRunning === b.isRunning &&
    a.activeProfileIndex === b.activeProfileIndex &&
    a.profileCount === b.profileCount &&
    (a.webrtcSink?.IsRunning ?? null) === (b.webrtcSink?.IsRunning ?? null) &&
    (a.nt4Sink?.IsRunning ?? null) === (b.nt4Sink?.IsRunning ?? null)
  );
}

export const PipelineNode = React.memo(PipelineNodeImpl, pipelineNodePropsEqual);

export const nodeTypes = { pipelineNode: PipelineNode };
