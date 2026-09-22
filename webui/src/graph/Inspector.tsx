import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { X, Trash2, Wifi, WifiOff, Radio, Play, Square, Code, RefreshCw, Wand2 } from 'lucide-react';
import type { PipelineNode } from './model';
import type { WsSource, WsSink, NT4Defaults } from '../types';
import { ApiService } from '../services/ApiService';
import { ToggleSwitch } from '../components/ToggleSwitch';
import { WebRTCStream } from '../components/WebRTCStream';

const api = new ApiService();

// ROADMAP.md Phase 8c: right-hand inspector on node selection - live parameters, a live
// preview (reusing WebRTCStream.tsx's connection logic as-is), the node's latest result JSON,
// and FPS/latency/backend already shown on the node card itself. Node creation/connection stay
// on the canvas (GraphPage); this panel is for configuring and observing a node once it exists.
export const Inspector: React.FC<{
  node: PipelineNode;
  onClose: () => void;
  onToast: (message: string, type: 'success' | 'error' | 'info') => void;
  onDeleted: () => void;
  nt4Settings: NT4Defaults;
}> = ({ node, onClose, onToast, onDeleted, nt4Settings }) => {
  const navigate = useNavigate();
  const { kind, raw, webrtcSink, nt4Sink, isRunning } = node.data;
  const [name, setName] = useState(node.data.label);
  const [resultJson, setResultJson] = useState<string | null>(null);
  const [showPreview, setShowPreview] = useState(false);
  const [newProfileName, setNewProfileName] = useState('');
  const [newProfileTagSize, setNewProfileTagSize] = useState(0.1651);

  useEffect(() => {
    setName(node.data.label);
    setResultJson(null);
    setShowPreview(false);
  }, [node.id]);

  const source = kind === 'source' ? (raw as WsSource) : null;
  const sink = kind === 'sink' ? (raw as WsSink) : null;

  const saveName = async () => {
    try {
      if (source) await api.changeSourceName(source.Id, name);
      else if (sink) await api.renameSink(sink.Id, name);
      onToast('Renamed', 'success');
    } catch {
      onToast('Rename failed', 'error');
    }
  };

  const handleDelete = async () => {
    if (!confirm(`Delete ${node.data.label}?`)) return;
    try {
      if (source) await api.deleteSource(source.Id);
      else if (sink) await api.deleteSink(sink.Id);
      onToast('Deleted', 'info');
      onDeleted();
    } catch {
      onToast('Delete failed', 'error');
    }
  };

  const toggleEnabled = async (enabled: boolean) => {
    if (!sink) return;
    try {
      await api.toggleSink(sink.Id, enabled);
    } catch {
      onToast('Failed to toggle sink', 'error');
    }
  };

  const togglePreview = async () => {
    if (!sink) return;
    try {
      if (webrtcSink) {
        await api.toggleSink(webrtcSink.Sink.Id, !webrtcSink.IsRunning);
      } else {
        const previewId = await api.createWebRTCSink(`${sink.Name}-preview`);
        await api.bindSinkToSource(previewId, sink.Id);
        await api.toggleSink(previewId, true);
      }
    } catch {
      onToast('Failed to toggle preview', 'error');
    }
  };

  const toggleNT4 = async () => {
    if (!sink) return;
    try {
      if (nt4Sink) {
        await api.toggleSink(nt4Sink.Sink.Id, !nt4Sink.IsRunning);
        return;
      }
      if (nt4Settings.mode === 'team' && !nt4Settings.teamNumber) {
        onToast('Set a NetworkTables team number in Settings first', 'error');
        return;
      }
      const ntId = nt4Settings.mode === 'team'
        ? await api.createNetworkTablesSinkForTeam(`${sink.Name}-nt4`, nt4Settings.teamNumber!, nt4Settings.rootTable)
        : await api.createNetworkTablesSinkForServer(`${sink.Name}-nt4`, nt4Settings.serverAddress!, nt4Settings.port, nt4Settings.rootTable);
      await api.bindSinkToSource(ntId, sink.Id);
      await api.toggleSink(ntId, true);
    } catch {
      onToast('Failed to toggle NT4 publish', 'error');
    }
  };

  const fetchResult = async () => {
    if (!sink) return;
    try {
      const result = await api.getSinkResult(sink.Id);
      setResultJson(JSON.stringify(result, null, 2));
    } catch (err) {
      setResultJson(`(failed to fetch result: ${err instanceof Error ? err.message : String(err)})`);
    }
  };

  const createApriltagProfile = async () => {
    if (!source || !newProfileName.trim()) return;
    try {
      await api.createApriltagProfile(source.Id, newProfileName.trim(), newProfileTagSize);
      setNewProfileName('');
      onToast('Profile created', 'success');
    } catch {
      onToast('Failed to create profile', 'error');
    }
  };

  const activateProfile = async (index: number) => {
    if (!source) return;
    try {
      await api.activateProfile(source.Id, index);
      onToast(`Activated profile ${index}`, 'success');
    } catch {
      onToast('Failed to activate profile', 'error');
    }
  };

  return (
    <div className="w-96 bg-white dark:bg-gray-800 border-l border-gray-200 dark:border-gray-700 flex flex-col h-full overflow-y-auto">
      <div className="p-4 border-b border-gray-200 dark:border-gray-700 flex items-center justify-between">
        <h3 className="font-semibold text-gray-900 dark:text-white">{node.data.typeName}</h3>
        <button onClick={onClose} className="text-gray-500 hover:text-gray-700"><X className="w-4 h-4" /></button>
      </div>

      <div className="p-4 space-y-4">
        <div>
          <label className="block text-xs font-medium text-gray-500 dark:text-gray-400 mb-1">Name</label>
          <div className="flex gap-2">
            <input value={name} onChange={e => setName(e.target.value)} onBlur={saveName}
              className="flex-1 px-2 py-1 text-sm border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white" />
          </div>
        </div>

        <div className="text-xs text-gray-500 dark:text-gray-400">
          ID: {source?.Id ?? sink?.Id} &middot; {node.data.fps.toFixed(1)} fps &middot; {(node.data.latencyUs / 1000).toFixed(1)} ms latency
        </div>

        {sink && (
          <>
            <div className="flex items-center justify-between">
              <span className="text-sm text-gray-700 dark:text-gray-300">Enabled</span>
              <ToggleSwitch enabled={isRunning ?? false} onChange={toggleEnabled} />
            </div>

            {/* ROADMAP.md Phase 8d: the only entry point into the calibration wizards - a
                CameraCalibrationSink/StereoCalibrationSink had no dedicated UI at all before
                this, only raw REST calls. */}
            {(node.data.typeName === 'CameraCalibrationSink' || node.data.typeName === 'StereoCalibrationSink') && (
              <button
                onClick={() => navigate(node.data.typeName === 'StereoCalibrationSink' ? `/calibrate/stereo/${sink.Id}` : `/calibrate/${sink.Id}`)}
                className="w-full px-3 py-2 bg-purple-600 hover:bg-purple-700 text-white rounded text-sm font-medium flex items-center justify-center gap-2"
              >
                <Wand2 className="w-4 h-4" />Open Calibration Wizard
              </button>
            )}

            <div className="flex items-center justify-between">
              <span className="text-sm text-gray-700 dark:text-gray-300">Live Preview</span>
              <button onClick={togglePreview} className={`px-2 py-1 rounded text-xs flex items-center gap-1 text-white ${webrtcSink?.IsRunning ? 'bg-red-600 hover:bg-red-700' : 'bg-green-600 hover:bg-green-700'}`}>
                {webrtcSink?.IsRunning ? <><Square className="w-3 h-3" />Stop</> : <><Play className="w-3 h-3" />Start</>}
              </button>
            </div>
            {webrtcSink?.IsRunning && (
              <WebRTCStream sinkId={webrtcSink.Sink.Id} onStop={togglePreview} onError={() => onToast('Preview stream error', 'error')} />
            )}

            <div className="flex items-center justify-between">
              <span className="text-sm text-gray-700 dark:text-gray-300 flex items-center gap-1"><Radio className="w-3 h-3" />Publish to NT4</span>
              <ToggleSwitch enabled={nt4Sink?.IsRunning ?? false} onChange={toggleNT4} />
            </div>

            <div>
              <button onClick={fetchResult} className="text-xs text-blue-600 hover:text-blue-700 flex items-center gap-1">
                <Code className="w-3 h-3" />{resultJson === null ? 'Fetch latest result' : 'Refresh result'}
              </button>
              {resultJson !== null && (
                <pre className="mt-2 p-2 text-xs bg-gray-100 dark:bg-gray-900 rounded overflow-x-auto max-h-64">{resultJson}</pre>
              )}
            </div>
          </>
        )}

        {source && source.Profiles.length >= 0 && (
          <div className="pt-2 border-t border-gray-200 dark:border-gray-700">
            <h4 className="text-xs font-medium text-gray-500 dark:text-gray-400 mb-2">Pipeline Profiles</h4>
            {source.Profiles.length === 0 ? (
              <p className="text-xs text-gray-400 mb-2">No profiles yet - a plain-created detector sink still works, profiles are only needed to switch between configurations at runtime.</p>
            ) : (
              <div className="space-y-1 mb-2">
                {source.Profiles.map(p => (
                  <div key={p.Index} className="flex items-center justify-between text-xs bg-gray-50 dark:bg-gray-700 rounded px-2 py-1">
                    <span>{p.Name} {p.TagSize != null && `(${p.TagSize}m)`}</span>
                    {source.ActiveProfileIndex === p.Index ? (
                      <span className="text-green-600 dark:text-green-400 font-medium">active</span>
                    ) : (
                      <button onClick={() => activateProfile(p.Index)} className="text-blue-600 hover:text-blue-700 flex items-center gap-1"><RefreshCw className="w-3 h-3" />Activate</button>
                    )}
                  </div>
                ))}
              </div>
            )}
            <div className="flex gap-2">
              <input value={newProfileName} onChange={e => setNewProfileName(e.target.value)} placeholder="New AprilTag profile name"
                className="flex-1 px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white" />
              <input type="number" step="any" value={newProfileTagSize} onChange={e => setNewProfileTagSize(parseFloat(e.target.value) || 0.1651)}
                className="w-20 px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white" />
              <button onClick={createApriltagProfile} className="px-2 py-1 bg-blue-600 text-white rounded text-xs hover:bg-blue-700">Add</button>
            </div>
          </div>
        )}

        <div className="pt-2 border-t border-gray-200 dark:border-gray-700">
          <button onClick={handleDelete} className="px-3 py-2 bg-red-600 text-white rounded text-sm hover:bg-red-700 flex items-center gap-2 w-full justify-center">
            <Trash2 className="w-4 h-4" />Delete
          </button>
        </div>
      </div>
    </div>
  );
};
