import { useState, useCallback } from 'react';
import type { Source, Sink, SystemStats, Toast, DeviceStats, AddSinkOptions, NT4Defaults } from '../types';
import { ApiService } from '../services/ApiService';

// Sink types that can themselves have a live preview or be published to NetworkTables -
// everything with a frame/json output, whether it's a raw Source or a dual-role detector Sink.
export const PUBLISHABLE_SINK_TYPES = ['apriltag', 'object', 'calibration'];

export const useAppData = () => {
  const [sources, setSources] = useState<Source[]>([]);
  const [sinks, setSinks] = useState<Sink[]>([]);
  const [streamingSinks, setStreamingSinks] = useState<Set<number>>(new Set());
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [deviceStats, setDeviceStats] = useState<DeviceStats>({
    cpuUsage: 0,
    ramUsage: 0,
    diskUsage: 0
  });
  const [systemStats, setSystemStats] = useState<SystemStats>({
    sources: 0,
    sinks: 0,
    activeStreams: 0,
    uptime: '00:00:00',
    serverStatus: 'offline'
  });
  const [toast, setToast] = useState<Toast | null>(null);
  
  const api = new ApiService();

  // Load device stats
  const loadDeviceStats = useCallback(async () => {
    try {
      const [cpuUsage, ramUsage, diskUsage] = await Promise.all([
        api.getDeviceCPUUsage().catch(() => 0),
        api.getDeviceRAMUsage().catch(() => 0),
        api.getDeviceDiskUsage().catch(() => 0)
      ]);
      
      setDeviceStats({ cpuUsage, ramUsage, diskUsage });
      return { cpuUsage, ramUsage, diskUsage };
    } catch (error) {
      console.warn('Failed to load device stats:', error);
      return { cpuUsage: 0, ramUsage: 0, diskUsage: 0 };
    }
  }, []);

  // helper to map server sink type enum to string label - must mirror Server.SinkType exactly
  // (Sink.cs): ApriltagSink=0, ObjectDetectionSink=1, RecordingSink=2, CameraCalibrationSink=3,
  // NetworkTablesSink=4, WebRTCSink=5, StereoCalibrationSink=6, StereoDepthSink=7,
  // DepthFusionSink=8.
  const mapSinkType = (type: any): string => {
    if (typeof type === 'string') return type;
    switch (type) {
      case 0: return 'apriltag';
      case 1: return 'object';
      case 2: return 'record';
      case 3: return 'calibration';
      case 4: return 'networktables';
      case 5: return 'webrtc';
      case 6: return 'stereocalibration';
      case 7: return 'stereodepth';
      case 8: return 'depthfusion';
      default: return 'unknown';
    }
  };

  // Load data with error handling
  const loadData = useCallback(async () => {
    try {
      setError(null);
      
      // Load all sources using the enhanced utility method
      const allSources = await api.getAllSources();
      
      // Convert to our Source type format
      const formattedSources: Source[] = allSources.map(source => ({
        id: source.Id || source.id,
        name: source.Name || source.name || `Source ${source.Id || source.id}`,
        type: source.Type || source.type || 'Unknown',
        status: 'active' as const,
        lastUpdate: new Date(),
        filePath: source.FilePath || source.filePath,
        fps: source.Fps || source.fps,
        cameraHardwareInfo: source.CameraHardwareInfo || source.cameraHardwareInfo
      }));

      // Load sinks from backend and map to UI type
      const serverSinks = await api.getAllSinks().catch(() => [] as any[]);
      const mappedSinks: Sink[] = await Promise.all(serverSinks.map(async s => {
        // Load sink status for each sink
        let isEnabled = false;
        try {
          isEnabled = await api.getSinkStatus(s.Id || s.id);
        } catch (error) {
          console.warn(`Failed to get status for sink ${s.Id || s.id}:`, error);
        }
        
        return {
          id: s.Id || s.id,
          name: s.Name || s.name || `Sink ${s.Id || s.id}`,
          type: mapSinkType(s.Type ?? s.type),
          status: 'active',
          lastUpdate: new Date(),
          sourceId: (s.Source && (s.Source.Id || s.Source.id)) || (s.source && (s.source.Id || s.source.id)),
          source2Id: (s.Source2 && (s.Source2.Id || s.Source2.id)) || (s.source2 && (s.source2.Id || s.source2.id)),
          isEnabled
        };
      }));
      
      // Load device stats
      const currentDeviceStats = await loadDeviceStats();

      setSources(formattedSources);
      setSinks(mappedSinks);
      
      // Update system stats with device stats
      setSystemStats({
        sources: formattedSources.length,
        sinks: mappedSinks.length,
        activeStreams: streamingSinks.size,
        uptime: new Date().toLocaleTimeString(),
        serverStatus: 'online',
        cpuUsage: currentDeviceStats.cpuUsage,
        ramUsage: currentDeviceStats.ramUsage,
        diskUsage: currentDeviceStats.diskUsage
      });
      
      setLoading(false);
    } catch (error) {
      console.error('Failed to load data:', error);
      setError(error instanceof Error ? error.message : 'Failed to load data');
      setSystemStats(prev => ({ ...prev, serverStatus: 'error' }));
      setLoading(false);
    }
  }, [streamingSinks.size, loadDeviceStats]);

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

  const handleAddSource = async (name: string, type: string, files?: FileList, fps?: number, hardwareInfo?: any) => {
    try {
      if (type === 'camera' && hardwareInfo) {
        const sourceId = await api.createCameraSource(hardwareInfo, name);
        showToast(`Camera source "${name}" added successfully with ID ${sourceId}`, 'success');
      } else if (type === 'video' && files && files.length > 0) {
        const result = await api.uploadVideoFiles(files, fps || 30);
        if (result.success) {
          showToast(result.message, 'success');
        } else {
          showToast(result.message, 'error');
          return;
        }
      } else if (type === 'image' && files && files.length > 0) {
        const result = await api.uploadImageFiles(files);
        if (result.success) {
          showToast(result.message, 'success');
        } else {
          showToast(result.message, 'error');
          return;
        }
      } else {
        throw new Error('Invalid source configuration');
      }
      
      // Refresh data to show the new sources
      setTimeout(() => loadData(), 1000);
    } catch (error) {
      showToast(`Failed to add source: ${error}`, 'error');
    }
  };

  const handleAddSink = async (name: string, type: string, options?: AddSinkOptions) => {
    try {
      let sinkId: number;

      switch (type) {
        case 'ApriltagSink':
          sinkId = await api.createApriltagSinkWithBackend(name, options?.tagSize ?? 0.1651, options?.backend ?? 0);
          break;

        case 'calibration':
          sinkId = await api.createCameraCalibrationSink(name);
          break;

        case 'object': {
          let modelId = options?.modelId;
          if (!modelId && options?.newModel) {
            modelId = await api.uploadModel(options.newModel);
          }
          if (!modelId) throw new Error('an object detection sink needs a model - upload one or pick an existing one');
          sinkId = await api.createObjectDetectionSink(name, modelId);
          break;
        }

        default:
          showToast(`Sink type "${type}" is not yet implemented.`, 'error');
          return;
      }

      // Refresh sinks from backend to keep UI consistent across reloads
      await loadData();

      showToast(`Sink "${name}" added successfully with ID ${sinkId}`, 'success');
    } catch (error) {
      showToast(`Failed to add sink: ${error}`, 'error');
    }
  };

  const handleRenameSource = async (id: number, name: string) => {
    try {
      await api.changeSourceName(id, name);
      showToast('Source renamed', 'success');
      loadData();
    } catch (e) {
      showToast('Failed to rename source', 'error');
    }
  };

  const handleDeleteSource = async (id: number) => {
    try {
      await api.deleteSource(id);
      showToast('Source deleted', 'info');
      loadData();
    } catch {
      showToast('Failed to delete source', 'error');
    }
  };

  const handleRenameSink = async (id: number, name: string) => {
    try {
      await api.renameSink(id, name);
      showToast('Sink renamed', 'success');
      loadData();
    } catch {
      showToast('Sink name changing not implemented yet', 'error');
    }
  };

  const handleDeleteSink = async (id: number) => {
    try {
      await api.deleteSink(id);
      
      // Reload from backend to reflect deletion consistently
      await loadData();
      showToast('Sink deleted', 'info');
    } catch {
      showToast('Failed to delete sink', 'error');
    }
  };

  const handleBindSink = async (sinkId: number, sourceId: number) => {
    try {
      await api.bindSinkToSource(sinkId, sourceId);
      
      // Update from backend
      await loadData();
      showToast('Sink bound to source', 'success');
    } catch {
      showToast('Failed to bind sink', 'error');
    }
  };

  const handleUnbindSink = async (sinkId: number, sourceId: number) => {
    try {
      await api.unbindSinkFromSource(sinkId, sourceId);
      
      // Update from backend
      await loadData();
      showToast('Sink unbound from source', 'info');
    } catch {
      showToast('Failed to unbind sink', 'error');
    }
  };

  // Enhanced bulk operations for multiple files
  const handleBulkVideoUpload = async (files: FileList, fps: number = 30) => {
    try {
      const result = await api.uploadVideoFiles(files, fps);
      if (result.success) {
        showToast(result.message, 'success');
        setTimeout(() => loadData(), 1000);
      } else {
        showToast(result.message, 'error');
      }
      return result;
    } catch (error) {
      const message = `Failed to upload video files: ${error}`;
      showToast(message, 'error');
      return { success: false, sourceIds: [], message };
    }
  };

  const handleBulkImageUpload = async (files: FileList) => {
    try {
      const result = await api.uploadImageFiles(files);
      if (result.success) {
        showToast(result.message, 'success');
        setTimeout(() => loadData(), 1000);
      } else {
        showToast(result.message, 'error');
      }
      return result;
    } catch (error) {
      const message = `Failed to upload image files: ${error}`;
      showToast(message, 'error');
      return { success: false, sourceIds: [], message };
    }
  };

  // UDP transmission controls
  const startUDPTransmission = async () => {
    try {
      await api.startUDPTransmission();
      showToast('UDP transmission started', 'success');
    } catch (error) {
      showToast(`Failed to start UDP transmission: ${error}`, 'error');
    }
  };

  const stopUDPTransmission = async () => {
    try {
      await api.stopUDPTransmission();
      showToast('UDP transmission stopped', 'info');
    } catch (error) {
      showToast(`Failed to stop UDP transmission: ${error}`, 'error');
    }
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
      await loadData();
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
      await loadData();
      showToast(`Publishing "${node.name}" to NetworkTables`, 'success');
    } catch (error) {
      showToast(`Failed to toggle NetworkTables publishing: ${error}`, 'error');
    }
  };

  const handleToggleSink = async (sinkId: number, enabled: boolean) => {
    try {
      await api.toggleSink(sinkId, enabled);
      
      // Update local state immediately for better UX
      setSinks(prevSinks => 
        prevSinks.map(sink => 
          sink.id === sinkId 
            ? { ...sink, isEnabled: enabled }
            : sink
        )
      );
      
      showToast(`Sink ${enabled ? 'enabled' : 'disabled'}`, 'success');
    } catch (error) {
      showToast(`Failed to toggle sink: ${error}`, 'error');
      // Reload data to revert to actual server state
      loadData();
    }
  };

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
    loadDeviceStats,
    stopStream,
    handleStreamError,
    showToast,
    handleAddSource,
    handleAddSink,
    handleRenameSource,
    handleDeleteSource,
    handleRenameSink,
    handleDeleteSink,
    handleBindSink,
    handleUnbindSink,
    handleBulkVideoUpload,
    handleBulkImageUpload,
    startUDPTransmission,
    stopUDPTransmission,
    handleToggleSink,
    handleTogglePreview,
    handleToggleNT4Publish,
    setStreamingSinks,
    setError,
    setToast
  };
};