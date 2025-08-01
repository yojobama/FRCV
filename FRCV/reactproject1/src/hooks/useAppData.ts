import { useState, useCallback } from 'react';
import type { Source, Sink, SystemStats, Toast } from '../types';
import { ApiService } from '../services/ApiService';

export const useAppData = () => {
  const [sources, setSources] = useState<Source[]>([]);
  const [sinks, setSinks] = useState<Sink[]>([]);
  const [streamingSinks, setStreamingSinks] = useState<Set<number>>(new Set());
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [systemStats, setSystemStats] = useState<SystemStats>({
    sources: 0,
    sinks: 0,
    activeStreams: 0,
    uptime: '00:00:00',
    serverStatus: 'offline'
  });
  const [toast, setToast] = useState<Toast | null>(null);
  
  const api = new ApiService();

  // Load data with error handling
  const loadData = useCallback(async () => {
    try {
      setError(null);
      const [sourceIds, sinkIds] = await Promise.all([
        api.getSources().catch(() => []),
        api.getSinks().catch(() => [])
      ]);

      // Load source details
      const sourceDetails = await Promise.all(
        sourceIds.map(async (id) => {
          try {
            const source = await api.getSource(id);
            return {
              id,
              name: source?.name || `Source ${id}`,
              type: source?.type || 'Unknown',
              status: 'active' as const,
              lastUpdate: new Date()
            };
          } catch {
            return {
              id,
              name: `Source ${id}`,
              type: 'Unknown',
              status: 'error' as const,
              lastUpdate: new Date()
            };
          }
        })
      );

      // Load sink details
      const sinkDetails = await Promise.all(
        sinkIds.map(async (id) => {
          try {
            const sink = await api.getSink(id);
            const streamStatus = await api.getWebRTCStatus(id);
            return {
              id,
              name: sink?.name || `Sink ${id}`,
              type: sink?.type || 'Unknown',
              status: 'active' as const,
              isStreaming: streamStatus?.isStreaming || false,
              lastUpdate: new Date(),
              sourceId: sink?.sourceId
            };
          } catch {
            return {
              id,
              name: `Sink ${id}`,
              type: 'Unknown',
              status: 'error' as const,
              isStreaming: false,
              lastUpdate: new Date()
            };
          }
        })
      );

      setSources(sourceDetails);
      setSinks(sinkDetails);
      
      // Update system stats
      setSystemStats({
        sources: sourceDetails.length,
        sinks: sinkDetails.length,
        activeStreams: streamingSinks.size,
        uptime: new Date().toLocaleTimeString(),
        serverStatus: 'online'
      });
      
      setLoading(false);
    } catch (error) {
      console.error('Failed to load data:', error);
      setError(error instanceof Error ? error.message : 'Failed to load data');
      setSystemStats(prev => ({ ...prev, serverStatus: 'error' }));
      setLoading(false);
    }
  }, [streamingSinks.size]);

  const startStream = (sinkId: number) => {
    setStreamingSinks(prev => new Set([...prev, sinkId]));
    showToast(`Started streaming for Sink ${sinkId}`, 'success');
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

  const handleAddSource = async (name: string, type: string, file?: File, fps?: number, hardwareInfo?: any) => {
    try {
      let sourceId: number;
      
      if (type === 'camera' && hardwareInfo) {
        sourceId = await api.createCameraSource(hardwareInfo);
      } else if (type === 'video' && file) {
        sourceId = await api.createVideoFileSource(file, fps || 30);
      } else if (type === 'image' && file) {
        sourceId = await api.createImageFileSource(file);
      } else {
        throw new Error('Invalid source configuration');
      }
      
      // Wait for the source to be created and start streaming
      await new Promise<void>((resolve, reject) => {
        const interval = setInterval(async () => {
          try {
            const source = await api.getSource(sourceId);
            if (source?.status === 'active') {
              clearInterval(interval);
              resolve();
            }
          } catch (error) {
            clearInterval(interval);
            reject(error);
          }
        }, 1000);
        
        // Timeout after 10 seconds
        setTimeout(() => {
          reject(new Error('Source creation timed out'));
        }, 10000);
      });
      
      showToast(`Source "${name}" added and started streaming successfully with ID ${sourceId}`, 'success');
      loadData(); // Refresh data
    } catch (error) {
      showToast(`Failed to add source: ${error}`, 'error');
    }
  };

  const handleAddSink = async (name: string, type: string) => {
    try {
      await api.addSink(name, type);
      showToast(`Sink "${name}" added successfully`, 'success');
      loadData(); // Refresh data
    } catch (error) {
      showToast(`Failed to add sink: ${error}`, 'error');
    }
  };

  return {
    sources,
    sinks,
    streamingSinks,
    loading,
    error,
    systemStats,
    toast,
    loadData,
    startStream,
    stopStream,
    handleStreamError,
    showToast,
    handleAddSource,
    handleAddSink,
    setStreamingSinks,
    setError,
    setToast
  };
};