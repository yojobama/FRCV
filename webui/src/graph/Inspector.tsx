import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { X, Trash2, Wifi, WifiOff, Radio, Play, Square, Code, RefreshCw, Wand2 } from 'lucide-react';
import type { PipelineNode } from './model';
import type { WsSource, WsSink, NT4Defaults, CameraMode } from '../types';
import { ApiService } from '../services/ApiService';
import { ToggleSwitch } from '../components/ToggleSwitch';
import { WebRTCStream } from '../components/WebRTCStream';

const api = new ApiService();

// LumenCore/FrameFormat.h's declaration order - see CameraMode's own comment in types/index.ts.
const PIXEL_FORMAT_NAMES = ['BGR24', 'RGB24', 'GRAY8', 'NV12', 'YUYV', 'MJPEG'];
const modeLabel = (m: CameraMode) => `${m.Width}x${m.Height} @ ${m.Fps}fps (${PIXEL_FORMAT_NAMES[m.PixelFormat] ?? m.PixelFormat})`;
const modeKey = (m: CameraMode) => `${m.Width}x${m.Height}x${m.Fps}x${m.PixelFormat}`;

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
  // 0 = CPU (apriltag), 1 = Vulkan (vkapriltag) - matches AddSinkModal's own convention. Used
  // both for the profile-creation dropdown below and the sink's own "Backend" control further
  // down (SinkManager.SetApriltagBackend rebuilds the sink in place to apply it).
  const [newProfileBackend, setNewProfileBackend] = useState(0);
  const [cameraModes, setCameraModes] = useState<CameraMode[]>([]);
  const [currentMode, setCurrentMode] = useState<CameraMode | null>(null);
  const [autoExposure, setAutoExposure] = useState(true);
  const [exposureValue, setExposureValue] = useState(300);
  const [gainValue, setGainValue] = useState(0);
  const [sinkBackend, setSinkBackend] = useState<number | null>(null);
  const [switchingBackend, setSwitchingBackend] = useState(false);
  // threads/quadDecimate are genuinely user-adjustable (not hardcoded - see ApriltagDetector's
  // own constructor comment). quadDecimateSupported is false for Vulkan (fixed 2x decimation
  // baked into its compute pipeline), so that control is hidden rather than accepting a value
  // that would be silently ignored.
  const [threadsValue, setThreadsValue] = useState(0);
  const [quadDecimateValue, setQuadDecimateValue] = useState(0);
  const [quadDecimateSupported, setQuadDecimateSupported] = useState(true);
  const [applyingTuning, setApplyingTuning] = useState(false);

  useEffect(() => {
    setName(node.data.label);
    setResultJson(null);
    setShowPreview(false);
    setCameraModes([]);
    setCurrentMode(null);
  }, [node.id]);

  const source = kind === 'source' ? (raw as WsSource) : null;
  const sink = kind === 'sink' ? (raw as WsSink) : null;
  const isCamera = source != null && source.Type === 0;

  // Modes/current mode aren't in the /ws/state snapshot (they're a live device query, not
  // pipeline state), so this needs its own fetch - only for camera sources, only once per
  // selected node rather than on every WS tick.
  useEffect(() => {
    if (!isCamera || !source) return;
    let cancelled = false;
    Promise.all([api.getCameraModes(source.Id), api.getCameraCurrentMode(source.Id)])
      .then(([modes, mode]) => {
        if (cancelled) return;
        setCameraModes(modes);
        setCurrentMode(mode);
      })
      .catch(() => { if (!cancelled) onToast('Failed to load camera modes', 'error'); });
    return () => { cancelled = true; };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [node.id, isCamera]);

  const changeMode = async (mode: CameraMode) => {
    if (!source) return;
    try {
      await api.setCameraMode(source.Id, mode);
      const applied = await api.getCameraCurrentMode(source.Id);
      setCurrentMode(applied);
      onToast(applied.IsNative ? 'Mode applied' : 'Camera substituted the nearest supported mode', applied.IsNative ? 'success' : 'info');
    } catch {
      onToast('Failed to set camera mode', 'error');
    }
  };

  const changeAutoExposure = async (enabled: boolean) => {
    if (!source) return;
    try {
      await api.setCameraAutoExposure(source.Id, enabled);
      setAutoExposure(enabled);
    } catch {
      onToast('Auto-exposure not supported by this device', 'error');
    }
  };

  const applyExposure = async () => {
    if (!source) return;
    try {
      await api.setCameraExposure(source.Id, exposureValue);
      onToast('Exposure applied', 'success');
    } catch {
      onToast('Exposure not supported by this device', 'error');
    }
  };

  const applyGain = async () => {
    if (!source) return;
    try {
      await api.setCameraGain(source.Id, gainValue);
      onToast('Gain applied', 'success');
    } catch {
      onToast('Gain not supported by this device', 'error');
    }
  };

  const isApriltagSink = sink != null && node.data.typeName === 'ApriltagSink';

  // Same reasoning as the camera modes fetch above - the sink's actual running backend/tuning
  // isn't in the /ws/state snapshot, so this needs its own one-shot fetch per selected node.
  useEffect(() => {
    if (!isApriltagSink || !sink) return;
    let cancelled = false;
    api.getApriltagBackendKind(sink.Id)
      .then(backend => { if (!cancelled) setSinkBackend(backend); })
      .catch(() => { if (!cancelled) onToast('Failed to load detector backend', 'error'); });
    api.getApriltagTuning(sink.Id)
      .then(tuning => {
        if (cancelled) return;
        setThreadsValue(tuning.threads);
        setQuadDecimateValue(tuning.quadDecimate);
        setQuadDecimateSupported(tuning.quadDecimateSupported);
      })
      .catch(() => { if (!cancelled) onToast('Failed to load detector tuning', 'error'); });
    return () => { cancelled = true; };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [node.id, isApriltagSink]);

  // Rebuilds the detector in place (SinkManager.SetApriltagBackend) - preserves id/tag size/
  // calibration/bindings, but the underlying native object is genuinely destroyed and recreated
  // (ApriltagDetector::m_Backend has no setter), so an open Live Preview may show a brief black
  // frame while its binding to the new detector re-establishes on the next /ws/state tick.
  // Carries the current threads/quadDecimate values forward explicitly so switching backend
  // doesn't reset tuning the user already dialled in (Vulkan simply ignores quadDecimate - fixed
  // 2x decimation - so passing it through on a switch to Vulkan is harmless).
  const switchBackend = async (backend: number) => {
    if (!sink) return;
    setSwitchingBackend(true);
    try {
      await api.setApriltagBackend(sink.Id, backend, threadsValue, quadDecimateValue);
      setSinkBackend(backend);
      setQuadDecimateSupported(backend !== 1);
      onToast('Backend switched', 'success');
    } catch {
      onToast('Failed to switch backend', 'error');
    } finally {
      setSwitchingBackend(false);
    }
  };

  // Applies threads/quadDecimate without changing backend - same rebuild-in-place mechanism.
  const applyTuning = async () => {
    if (!sink || sinkBackend === null) return;
    setApplyingTuning(true);
    try {
      await api.setApriltagBackend(sink.Id, sinkBackend, threadsValue, quadDecimateSupported ? quadDecimateValue : undefined);
      onToast('Tuning applied', 'success');
    } catch {
      onToast('Failed to apply tuning', 'error');
    } finally {
      setApplyingTuning(false);
    }
  };

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
      await api.createApriltagProfile(source.Id, newProfileName.trim(), newProfileTagSize, { backend: newProfileBackend });
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

        {isCamera && (
          <div className="pt-2 border-t border-gray-200 dark:border-gray-700 space-y-3">
            <h4 className="text-xs font-medium text-gray-500 dark:text-gray-400">Camera Controls</h4>

            <div>
              <label className="block text-xs text-gray-500 dark:text-gray-400 mb-1">
                Resolution / FPS {currentMode && !currentMode.IsNative && <span className="text-yellow-500">(substituted)</span>}
              </label>
              {cameraModes.length > 0 ? (
                <select
                  value={currentMode ? modeKey(currentMode) : ''}
                  onChange={e => {
                    const mode = cameraModes.find(m => modeKey(m) === e.target.value);
                    if (mode) changeMode(mode);
                  }}
                  className="w-full px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white"
                >
                  {currentMode && !cameraModes.some(m => modeKey(m) === modeKey(currentMode!)) && (
                    <option value={modeKey(currentMode)}>{modeLabel(currentMode)} (current)</option>
                  )}
                  {cameraModes.map(m => (
                    <option key={modeKey(m)} value={modeKey(m)}>{modeLabel(m)}</option>
                  ))}
                </select>
              ) : (
                <p className="text-xs text-gray-400">
                  {currentMode ? modeLabel(currentMode) : 'Loading modes...'}
                  {cameraModes.length === 0 && currentMode && ' - device reports no other selectable modes'}
                </p>
              )}
            </div>

            <div className="flex items-center justify-between">
              <span className="text-xs text-gray-700 dark:text-gray-300">Auto Exposure</span>
              <ToggleSwitch enabled={autoExposure} onChange={changeAutoExposure} />
            </div>

            {/* a fixed short exposure is what actually makes AprilTag detection reliable on a
                moving robot (motion blur otherwise smears the tag edges) - this is the whole
                point of exposing manual exposure control here, not just a nice-to-have. */}
            <div className={autoExposure ? 'opacity-50 pointer-events-none' : ''}>
              <label className="block text-xs text-gray-500 dark:text-gray-400 mb-1">Exposure (100&micro;s units)</label>
              <div className="flex gap-2">
                <input type="number" value={exposureValue} onChange={e => setExposureValue(parseInt(e.target.value) || 0)}
                  className="flex-1 px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white" />
                <button onClick={applyExposure} className="px-2 py-1 bg-blue-600 text-white rounded text-xs hover:bg-blue-700">Apply</button>
              </div>
            </div>

            <div>
              <label className="block text-xs text-gray-500 dark:text-gray-400 mb-1">Gain</label>
              <div className="flex gap-2">
                <input type="number" value={gainValue} onChange={e => setGainValue(parseInt(e.target.value) || 0)}
                  className="flex-1 px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white" />
                <button onClick={applyGain} className="px-2 py-1 bg-blue-600 text-white rounded text-xs hover:bg-blue-700">Apply</button>
              </div>
            </div>

            <p className="text-xs text-gray-400">
              Not every device/driver honours all of these - a control this camera doesn't support fails with a toast rather than silently doing nothing.
            </p>
          </div>
        )}

        {sink && (
          <>
            <div className="flex items-center justify-between">
              <span className="text-sm text-gray-700 dark:text-gray-300">Enabled</span>
              <ToggleSwitch enabled={isRunning ?? false} onChange={toggleEnabled} />
            </div>

            {/* directly on the sink, not just buried in Pipeline Profiles - SetApriltagBackend
                rebuilds the detector in place (same id/tag size/calibration/bindings), so this
                works on any ApriltagSink whether or not it was ever set up via a profile. */}
            {isApriltagSink && (
              <div>
                <label className="block text-xs text-gray-500 dark:text-gray-400 mb-1">Backend</label>
                <select
                  value={sinkBackend ?? 0}
                  disabled={sinkBackend === null || switchingBackend}
                  onChange={e => switchBackend(parseInt(e.target.value))}
                  className="w-full px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white disabled:opacity-50"
                >
                  <option value={0}>CPU (apriltag)</option>
                  <option value={1}>Vulkan (vkapriltag)</option>
                </select>
              </div>
            )}

            {/* genuinely adjustable, not hardcoded - see ApriltagDetector's own constructor
                comment. Threads applies to both backends (libapriltag's nthreads / vkapriltag's
                cpu_threads); QuadDecimate is CPU-only (Vulkan's decimation is architecturally
                fixed at 2x) and is hidden rather than accepting a value that'd be silently
                ignored. */}
            {isApriltagSink && (
              <div className="space-y-2">
                <div>
                  <label className="block text-xs text-gray-500 dark:text-gray-400 mb-1">Threads (0 = default)</label>
                  <input type="number" min={0} value={threadsValue} onChange={e => setThreadsValue(parseInt(e.target.value) || 0)}
                    className="w-full px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white" />
                </div>
                {quadDecimateSupported ? (
                  <div>
                    <label className="block text-xs text-gray-500 dark:text-gray-400 mb-1">Quad Decimate (0 = default)</label>
                    <input type="number" min={0} step={0.5} value={quadDecimateValue}
                      onChange={e => setQuadDecimateValue(parseFloat(e.target.value) || 0)}
                      className="w-full px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white" />
                  </div>
                ) : (
                  <p className="text-xs text-gray-400">Vulkan's decimation is fixed at 2x - not adjustable.</p>
                )}
                <button onClick={applyTuning} disabled={applyingTuning}
                  className="w-full px-2 py-1 bg-blue-600 text-white rounded text-xs hover:bg-blue-700 disabled:opacity-50">Apply Tuning</button>
              </div>
            )}

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
            <div className="space-y-1">
              <input value={newProfileName} onChange={e => setNewProfileName(e.target.value)} placeholder="New AprilTag profile name"
                className="w-full px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white" />
              <div className="flex gap-2">
                <input type="number" step="any" value={newProfileTagSize} onChange={e => setNewProfileTagSize(parseFloat(e.target.value) || 0.1651)}
                  title="Tag size (meters)"
                  className="w-20 px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white" />
                <select value={newProfileBackend} onChange={e => setNewProfileBackend(parseInt(e.target.value))}
                  className="flex-1 px-2 py-1 text-xs border border-gray-300 dark:border-gray-600 rounded dark:bg-gray-700 dark:text-white">
                  <option value={0}>CPU (apriltag)</option>
                  <option value={1}>Vulkan (vkapriltag)</option>
                </select>
                <button onClick={createApriltagProfile} className="px-2 py-1 bg-blue-600 text-white rounded text-xs hover:bg-blue-700">Add</button>
              </div>
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
