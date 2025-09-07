import { useState, useCallback } from 'react';
import type { Source, Sink, SystemStats, Toast, DeviceStats } from '../types';
import { ApiService } from '../services/ApiService';

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

      // Load sinks (currently limited since there's no general endpoint)
      const sinkDetails: Sink[] = [];
      // Since we can't get all sinks, we start with an empty array
      // Sinks will be populated as they are created through the UI
      
      // Load device stats
      const currentDeviceStats = await loadDeviceStats();

      setSources(formattedSources);
      setSinks(sinkDetails);
      
      // Update system stats with device stats
      setSystemStats({
        sources: formattedSources.length,
        sinks: sinkDetails.length,
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

  const startStream = (sinkId: number) => {
    // Note: WebRTC streaming is not implemented in current API
    showToast(`WebRTC streaming not yet implemented for Sink ${sinkId}`, 'info');
  };

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
        const sourceId = await api.createCameraSource(hardwareInfo);
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

  const handleAddSink = async (name: string, type: string) => {
    try {
      // Currently only AprilTag sinks are implemented
      if (type !== 'apriltag') {
        showToast(`Sink type "${type}" is not yet implemented. Only AprilTag sinks are available.`, 'error');
        return;
      }
      
      const sinkId = await api.createApriltagSink(name, type);
      
      // Since we can't reload all sinks, we'll add the new sink to our local state
      const newSink: Sink = {
        id: sinkId,
        name: name,
        type: type,
        status: 'active',
        lastUpdate: new Date()
      };
      
      setSinks(prev => [...prev, newSink]);
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
      await api.changeSinkName(id, name);
      showToast('Sink renamed', 'success');
      loadData();
    } catch {
      showToast('Sink name changing not implemented yet', 'error');
    }
  };

  const handleDeleteSink = async (id: number) => {
    try {
      await api.deleteSink(id);
      
      // Remove from local state since we can't reload all sinks
      setSinks(prev => prev.filter(sink => sink.id !== id));
      showToast('Sink deleted', 'info');
    } catch {
      showToast('Failed to delete sink', 'error');
    }
  };

  const handleBindSink = async (sinkId: number, sourceId: number) => {
    try {
      await api.bindSinkToSource(sinkId, sourceId);
      
      // Update local state
      setSinks(prev => prev.map(sink => 
        sink.id === sinkId ? { ...sink, sourceId } : sink
      ));
      showToast('Sink bound to source', 'success');
    } catch {
      showToast('Failed to bind sink', 'error');
    }
  };

  const handleUnbindSink = async (sinkId: number, sourceId: number) => {
    try {
      await api.unbindSinkFromSource(sinkId, sourceId);
      
      // Update local state
      setSinks(prev => prev.map(sink => 
        sink.id === sinkId ? { ...sink, sourceId: undefined } : sink
      ));
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
    startStream,
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
    setStreamingSinks,
    setError,
    setToast
  };
};