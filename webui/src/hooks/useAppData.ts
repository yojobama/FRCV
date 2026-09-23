import { useEffect, useMemo, useState } from 'react';
import type { Source, Sink, SystemStats, Toast, DeviceStats, NT4Defaults } from '../types';
import { ApiService } from '../services/ApiService';
import { useStateSocket } from './useStateSocket';

// Maps the server's SinkType enum ordinal to the string label this UI uses everywhere - must
// mirror Server/Sink.cs's SinkType exactly: ApriltagSink=0, ObjectDetectionSink=1,
// (2 reserved - was RecordingSink, deleted, never reused - see Sink.cs's own comment),
// CameraCalibrationSink=3, NetworkTablesSink=4, WebRTCSink=5, StereoCalibrationSink=6,
// StereoDepthSink=7, DepthFusionSink=8, MjpegSink=9. Exported (not just a local helper) so
// Phase 8c's graph code can reuse it without a second copy of this enum-order knowledge.
export const mapSinkType = (type: any): string => {
  if (typeof type === 'string') return type;
  switch (type) {
    case 0: return 'apriltag';
    case 1: return 'object';
    case 3: return 'calibration';
    case 4: return 'networktables';
    case 5: return 'webrtc';
    case 6: return 'stereocalibration';
    case 7: return 'stereodepth';
    case 8: return 'depthfusion';
    case 9: return 'mjpeg';
    default: return 'unknown';
  }
};

// Same story as mapSinkType, for Server/Source.cs's SourceType enum: Camera=0, ImageFile=1,
// VideoFile=2, SinkOutput=3.
const mapSourceType = (type: any): string => {
  if (typeof type === 'string') return type;
  switch (type) {
    case 0: return 'camera';
    case 1: return 'image';
    case 2: return 'video';
    case 3: return 'sinkoutput';
    default: return 'unknown';
  }
};

// ROADMAP.md Phase 8/E6: Dashboard/StereoPage's data now comes from the same /ws/state push
// channel the Graph/Match pages already use (useStateSocket), not this hook's own REST polling
// loop - one snapshot per server tick instead of getAllSources+getAllSinks+N*getSinkStatus+3
// device-stat calls per client per refresh. What's left of the REST-based ApiService calls here
// are genuine ACTIONS (toggle/create/bind a sink), not data reads - those still need a real
// request/response round trip, a push channel has nothing to push until the action completes.
export const useAppData = () => {
  const { snapshot, connected } = useStateSocket();
  const [streamingSinks, setStreamingSinks] = useState<Set<number>>(new Set());
  const [error, setError] = useState<string | null>(null);
  const [toast, setToast] = useState<Toast | null>(null);

  const api = new ApiService();

  // derived, not stored in its own useState+useEffect pair - sources/sinks/deviceStats/
  // systemStats are all pure functions of the latest snapshot, so there's nothing to
  // "synchronize" here the way the old REST-polling version had to.
  const sources: Source[] = useMemo(() => {
    if (!snapshot) return [];
    return snapshot.Sources.map(s => ({
      id: s.Id,
      name: s.Name || `Source ${s.Id}`,
      type: mapSourceType(s.Type),
      status: 'active' as const,
      lastUpdate: new Date(),
      filePath: s.FilePath ?? undefined,
      fps: s.Fps ?? undefined,
      cameraHardwareInfo: s.CameraHardwareInfo
        ? { Name: s.CameraHardwareInfo.name, Path: s.CameraHardwareInfo.path }
        : undefined,
    }));
  }, [snapshot]);

  const sinks: Sink[] = useMemo(() => {
    if (!snapshot) return [];
    return snapshot.Sinks.map(({ Sink: s, IsRunning }) => ({
      id: s.Id,
      name: s.Name || `Sink ${s.Id}`,
      type: mapSinkType(s.Type),
      status: 'active' as const,
      lastUpdate: new Date(),
      sourceId: s.Source?.Id,
      source2Id: s.Source2?.Id,
      isEnabled: IsRunning,
    }));
  }, [snapshot]);

  // MB, not bytes - WsDeviceStats already reports RamUsageMb directly (unlike the old
  // getDeviceRAMUsage() REST call, which returned raw bytes) - see DeviceStats's own comment.
  const deviceStats: DeviceStats = useMemo(() => ({
    cpuUsage: snapshot?.Device.CpuUsagePercent ?? 0,
    ramUsage: snapshot?.Device.RamUsageMb ?? 0,
    diskUsage: snapshot?.Device.DiskUsagePercent ?? 0,
  }), [snapshot]);

  const systemStats: SystemStats = useMemo(() => ({
    sources: sources.length,
    sinks: sinks.length,
    activeStreams: streamingSinks.size,
    uptime: new Date().toLocaleTimeString(),
    serverStatus: snapshot ? 'online' : (connected ? 'online' : 'offline'),
  }), [sources.length, sinks.length, streamingSinks.size, snapshot, connected]);

  // true only until the first snapshot arrives - after that the app is always showing SOME
  // data, live-updated, even across a brief disconnect/reconnect (useStateSocket keeps the last
  // snapshot around rather than clearing it, so a flaky connection doesn't blank the whole UI).
  const loading = snapshot === null;

  const stopStream = (sinkId: number) => {
    setStreamingSinks(prev => {
      const newSet = new Set(prev);
      newSet.delete(sinkId);
      return newSet;
    });
    showToast(`Stopped streaming for Sink ${sinkId}`, 'info');
  };

  const handleStreamError = (sinkId: number, error: string) => {
    stopStream(sinkId);
    showToast(`Stream error for Sink ${sinkId}: ${error}`, 'error');
  };

  const showToast = (message: string, type: 'success' | 'error' | 'info') => {
    setToast({ message, type });
  };

  // "Live Preview" toggle for any node (a raw Source, or a dual-role detector Sink) - creates a
  // dedicated WebRTCSink bound to it on first use rather than requiring the user to create and
  // bind one manually. WebRTC is not itself a user-facing addable sink type anymore; this is
  // what replaced that.
  const handleTogglePreview = async (node: { id: number; name: string }) => {
    try {
      const companion = sinks.find(s => s.type === 'webrtc' && s.sourceId === node.id);
      if (companion) {
        if (streamingSinks.has(companion.id)) {
          stopStream(companion.id);
          return;
        }
        if (!companion.isEnabled) {
          await api.toggleSink(companion.id, true);
        }
        setStreamingSinks(prev => new Set(prev).add(companion.id));
        return;
      }

      const sinkId = await api.createWebRTCSink(`${node.name}-preview`);
      await api.bindSinkToSource(sinkId, node.id);
      await api.toggleSink(sinkId, true);
      setStreamingSinks(prev => new Set(prev).add(sinkId));
    } catch (error) {
      showToast(`Failed to start preview: ${error}`, 'error');
    }
  };

  // "Publish to NetworkTables" toggle for a detector-type sink (apriltag/object/calibration) -
  // creates a dedicated NetworkTablesSink bound to it on first use, reusing the connection
  // details configured once in Settings. NetworkTables is not itself a user-facing addable sink
  // type anymore; this is what replaced that.
  const handleToggleNT4Publish = async (node: { id: number; name: string }, nt4: NT4Defaults) => {
    try {
      const companion = sinks.find(s => s.type === 'networktables' && s.sourceId === node.id);
      if (companion) {
        await handleToggleSink(companion.id, !companion.isEnabled);
        return;
      }

      if (nt4.mode === 'team' && !nt4.teamNumber) {
        showToast('Set a NetworkTables team number in Settings first', 'error');
        return;
      }
      if (nt4.mode === 'server' && !nt4.serverAddress) {
        showToast('Set a NetworkTables server address in Settings first', 'error');
        return;
      }

      const sinkId = nt4.mode === 'server'
        ? await api.createNetworkTablesSinkForServer(`${node.name}-nt4`, nt4.serverAddress!, nt4.port ?? 0, nt4.rootTable)
        : await api.createNetworkTablesSinkForTeam(`${node.name}-nt4`, nt4.teamNumber!, nt4.rootTable);
      await api.bindSinkToSource(sinkId, node.id);
      await api.toggleSink(sinkId, true);
      showToast(`Publishing "${node.name}" to NetworkTables`, 'success');
    } catch (error) {
      showToast(`Failed to toggle NetworkTables publishing: ${error}`, 'error');
    }
  };

  const handleToggleSink = async (sinkId: number, enabled: boolean) => {
    try {
      await api.toggleSink(sinkId, enabled);
      showToast(`Sink ${enabled ? 'enabled' : 'disabled'}`, 'success');
      // no local optimistic-update / reload-on-failure needed any more - the next /ws/state
      // tick (well under a second away) reflects the real server-side result either way.
    } catch (error) {
      showToast(`Failed to toggle sink: ${error}`, 'error');
    }
  };

  useEffect(() => {
    if (!connected) setError('Reconnecting to the server...');
    else setError(null);
  }, [connected]);

  // vestigial no-op, kept only so existing call sites (Header's "Refresh" button, three actions
  // in StereoPage.tsx that used to force an immediate reload after a mutation) don't need their
  // own separate cleanup pass - there is nothing left to "refresh" now that sources/sinks/
  // deviceStats/systemStats are all live-derived from the socket snapshot above; the next
  // /ws/state tick (well under a second away) already reflects any change on its own.
  const loadData = () => {};

  return {
    sources,
    sinks,
    streamingSinks,
    loading,
    error,
    systemStats,
    deviceStats,
    toast,
    loadData,
    stopStream,
    handleStreamError,
    showToast,
    handleToggleSink,
    handleTogglePreview,
    handleToggleNT4Publish,
    setStreamingSinks,
    setError,
    setToast
  };
};
