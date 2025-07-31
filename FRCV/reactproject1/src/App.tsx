import React, { useState, useEffect, useCallback } from 'react';
import './App.css';

// Types
interface Source {
  id: number;
  name: string;
  type: string;
  status: 'active' | 'inactive' | 'error';
  lastUpdate?: Date;
}

interface Sink {
  id: number;
  name: string;
  type: string;
  status: 'active' | 'inactive' | 'error';
  isStreaming?: boolean;
  lastUpdate?: Date;
  sourceId?: number;
}

interface CameraHardwareInfo {
  name: string;
  path: string;
}

interface SystemStats {
  sources: number;
  sinks: number;
  activeStreams: number;
  uptime: string;
  serverStatus: 'online' | 'offline' | 'error';
}

// API Service
class ApiService {
  private baseUrl = 'http://localhost:8175/api';

  async getSources(): Promise<number[]> {
    const response = await fetch(`${this.baseUrl}/source/ids`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getSinks(): Promise<number[]> {
    const response = await fetch(`${this.baseUrl}/sink/ids`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getSink(id: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/getSinkById?sinkId=${id}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getSource(id: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/source?sourceId=${id}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const text = await response.text();
    return text ? JSON.parse(text) : null;
  }

  async getCameraHardware(): Promise<CameraHardwareInfo[]> {
    const response = await fetch(`${this.baseUrl}/source/camera/hardwareInfos`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async enableSink(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/enable?sinkId=${id}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async disableSink(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/disable?sinkId=${id}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async addSink(name: string, type: string): Promise<number> {
    const response = await fetch(`${this.baseUrl}/sink/add?name=${encodeURIComponent(name)}&type=${encodeURIComponent(type)}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async deleteSink(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/delete?sinkId=${id}`, { method: 'DELETE' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async bindSinkToSource(sinkId: number, sourceId: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/bind?sinkId=${sinkId}&sourceId=${sourceId}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async startWebRTCStream(sinkId: number): Promise<any> {
    const connectionId = `conn_${Math.random().toString(36).substr(2, 9)}`;
    const response = await fetch(`${this.baseUrl}/sink/webrtc/start?sinkId=${sinkId}&connectionId=${connectionId}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async stopWebRTCStream(sinkId: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/webrtc/stop?sinkId=${sinkId}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getWebRTCStatus(sinkId: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/webrtc/status?sinkId=${sinkId}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async enablePreview(sinkId: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/preview/enable?sinkId=${sinkId}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async disablePreview(sinkId: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/preview/disable?sinkId=${sinkId}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }
}

// Enhanced WebRTC Stream Component
const WebRTCStream: React.FC<{ 
  sinkId: number; 
  onStop: () => void; 
  onError: (error: string) => void;
  className?: string;
}> = ({ sinkId, onStop, onError, className = '' }) => {
  const [connectionState, setConnectionState] = useState<string>('connecting');
  const [peerConnection, setPeerConnection] = useState<RTCPeerConnection | null>(null);
  const [isFullscreen, setIsFullscreen] = useState(false);
  const videoRef = React.useRef<HTMLVideoElement>(null);
  const containerRef = React.useRef<HTMLDivElement>(null);
  const api = new ApiService();

  useEffect(() => {
    startWebRTCConnection();
    return () => {
      if (peerConnection) {
        peerConnection.close();
      }
    };
  }, [sinkId]);

  const startWebRTCConnection = async () => {
    try {
      setConnectionState('connecting');
      
      // Enable preview first
      await api.enablePreview(sinkId);

      const config = {
        iceServers: [
          { urls: 'stun:stun.l.google.com:19302' },
          { urls: 'stun:stun1.l.google.com:19302' }
        ]
      };

      const pc = new RTCPeerConnection(config);
      setPeerConnection(pc);

      pc.onconnectionstatechange = () => {
        setConnectionState(pc.connectionState);
        if (pc.connectionState === 'failed' || pc.connectionState === 'disconnected') {
          onError(`Connection ${pc.connectionState}`);
        }
      };

      pc.ontrack = (event) => {
        if (videoRef.current && event.streams[0]) {
          videoRef.current.srcObject = event.streams[0];
          setConnectionState('connected');
        }
      };

      pc.onicecandidate = async (event) => {
        if (event.candidate) {
          try {
            await fetch(`http://localhost:8175/api/sink/webrtc/ice`, {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({
                sinkId: sinkId,
                connectionId: `conn_${sinkId}`,
                candidate: {
                  candidate: event.candidate.candidate,
                  sdpMid: event.candidate.sdpMid,
                  sdpMLineIndex: event.candidate.sdpMLineIndex
                }
              })
            });
          } catch (err) {
            console.warn('Failed to send ICE candidate:', err);
          }
        }
      };

      // Start stream
      const response = await api.startWebRTCStream(sinkId);
      if (response.success && response.offer) {
        await pc.setRemoteDescription(response.offer);
        const answer = await pc.createAnswer();
        await pc.setLocalDescription(answer);
        
        // Send answer back to server
        await fetch(`http://localhost:8175/api/sink/webrtc/answer`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            sinkId: sinkId,
            connectionId: response.connectionId,
            answer: { type: answer.type, sdp: answer.sdp }
          })
        });
      } else {
        throw new Error(response.error || 'Failed to start stream');
      }
    } catch (error) {
      console.error('WebRTC connection failed:', error);
      setConnectionState('failed');
      onError(error instanceof Error ? error.message : 'Connection failed');
    }
  };

  const stopStream = async () => {
    try {
      if (peerConnection) {
        peerConnection.close();
      }
      await api.stopWebRTCStream(sinkId);
      await api.disablePreview(sinkId);
    } catch (error) {
      console.warn('Error stopping stream:', error);
    } finally {
      onStop();
    }
  };

  const toggleFullscreen = () => {
    if (!document.fullscreenElement && containerRef.current) {
      containerRef.current.requestFullscreen();
      setIsFullscreen(true);
    } else if (document.fullscreenElement) {
      document.exitFullscreen();
      setIsFullscreen(false);
    }
  };

  useEffect(() => {
    const handleFullscreenChange = () => {
      setIsFullscreen(!!document.fullscreenElement);
    };
    document.addEventListener('fullscreenchange', handleFullscreenChange);
    return () => document.removeEventListener('fullscreenchange', handleFullscreenChange);
  }, []);

  const getStatusColor = (state: string) => {
    switch (state) {
      case 'connected': return 'bg-green-600';
      case 'connecting': return 'bg-yellow-600';
      case 'failed': return 'bg-red-600';
      default: return 'bg-gray-600';
    }
  };

  const getStatusText = (state: string) => {
    switch (state) {
      case 'connected': return 'Live';
      case 'connecting': return 'Connecting...';
      case 'failed': return 'Failed';
      default: return 'Unknown';
    }
  };

  return (
    <div ref={containerRef} className={`bg-black rounded-lg overflow-hidden ${className}`}>
      <div className="p-4 bg-gray-800 flex justify-between items-center">
        <div className="flex items-center space-x-3">
          <span className="text-white font-medium">Sink {sinkId} Stream</span>
          <span className={`px-2 py-1 rounded text-xs text-white ${getStatusColor(connectionState)} animate-pulse-slow`}>
            {getStatusText(connectionState)}
          </span>
        </div>
        
        <div className="flex items-center gap-2">
          <button
            onClick={toggleFullscreen}
            className="px-3 py-1 bg-blue-600 text-white rounded text-sm hover:bg-blue-700 transition-colors"
            title="Toggle fullscreen"
          >
            {isFullscreen ? '?' : '?'}
          </button>
          <button
            onClick={stopStream}
            className="px-3 py-1 bg-red-600 text-white rounded text-sm hover:bg-red-700 transition-colors"
            title="Stop stream"
          >
            Stop
          </button>
        </div>
      </div>
      
      <div className="relative">
        <video
          ref={videoRef}
          autoPlay
          muted
          playsInline
          className="w-full h-64 object-cover bg-black"
          style={{ aspectRatio: '16/9' }}
        />
        
        {connectionState === 'connecting' && (
          <div className="absolute inset-0 flex items-center justify-center bg-black bg-opacity-75">
            <div className="text-center text-white">
              <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-white mx-auto mb-2"></div>
              <p className="text-sm">Connecting to stream...</p>
            </div>
          </div>
        )}
        
        {connectionState === 'failed' && (
          <div className="absolute inset-0 flex items-center justify-center bg-red-900 bg-opacity-75">
            <div className="text-center text-white">
              <p className="text-sm">? Connection failed</p>
              <button 
                onClick={startWebRTCConnection}
                className="mt-2 px-3 py-1 bg-white text-red-900 rounded text-sm hover:bg-gray-100"
              >
                Retry
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

// Settings Modal Component
const SettingsModal: React.FC<{ 
  isOpen: boolean; 
  onClose: () => void; 
  settings: any; 
  onSave: (settings: any) => void;
}> = ({ isOpen, onClose, settings, onSave }) => {
  const [localSettings, setLocalSettings] = useState(settings);

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white dark:bg-gray-800 rounded-lg p-6 w-full max-w-md mx-4">
        <h2 className="text-xl font-bold text-gray-900 dark:text-white mb-4">Settings</h2>
        
        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
              Server URL
            </label>
            <input
              type="text"
              value={localSettings.serverUrl}
              onChange={(e) => setLocalSettings({...localSettings, serverUrl: e.target.value})}
              className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white"
            />
          </div>
          
          <div>
            <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
              Refresh Interval (seconds)
            </label>
            <input
              type="number"
              min="1"
              max="60"
              value={localSettings.refreshInterval}
              onChange={(e) => setLocalSettings({...localSettings, refreshInterval: parseInt(e.target.value)})}
              className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white"
            />
          </div>
        </div>
        
        <div className="flex justify-end space-x-3 mt-6">
          <button
            onClick={onClose}
            className="px-4 py-2 text-gray-600 dark:text-gray-400 hover:text-gray-800 dark:hover:text-gray-200"
          >
            Cancel
          </button>
          <button
            onClick={() => {
              onSave(localSettings);
              onClose();
            }}
            className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700"
          >
            Save
          </button>
        </div>
      </div>
    </div>
  );
};

// Toast Notification Component
const Toast: React.FC<{ 
  message: string; 
  type: 'success' | 'error' | 'info'; 
  onClose: () => void;
}> = ({ message, type, onClose }) => {
  useEffect(() => {
    const timer = setTimeout(onClose, 5000);
    return () => clearTimeout(timer);
  }, [onClose]);

  const getToastColor = () => {
    switch (type) {
      case 'success': return 'bg-green-600';
      case 'error': return 'bg-red-600';
      case 'info': return 'bg-blue-600';
      default: return 'bg-gray-600';
    }
  };

  return (
    <div className={`fixed top-4 right-4 ${getToastColor()} text-white px-4 py-2 rounded-lg shadow-lg z-50 animate-slide-up`}>
      <div className="flex items-center justify-between">
        <span>{message}</span>
        <button onClick={onClose} className="ml-3 text-white hover:text-gray-200">
          ?
        </button>
      </div>
    </div>
  );
};

// Main App Component
function App() {
  const [darkMode, setDarkMode] = useState(() => {
    const saved = localStorage.getItem('darkMode');
    return saved ? JSON.parse(saved) : false;
  });
  
  const [currentTab, setCurrentTab] = useState('dashboard');
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
  const [showSettings, setShowSettings] = useState(false);
  const [settings, setSettings] = useState({
    serverUrl: 'http://localhost:8175',
    refreshInterval: 5
  });
  const [toast, setToast] = useState<{message: string; type: 'success' | 'error' | 'info'} | null>(null);
  
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

  // Initialize data loading
  useEffect(() => {
    loadData();
    const interval = setInterval(loadData, settings.refreshInterval * 1000);
    return () => clearInterval(interval);
  }, [loadData, settings.refreshInterval]);

  // Dark mode management
  useEffect(() => {
    localStorage.setItem('darkMode', JSON.stringify(darkMode));
    if (darkMode) {
      document.documentElement.classList.add('dark');
    } else {
      document.documentElement.classList.remove('dark');
    }
  }, [darkMode]);

  const toggleDarkMode = () => {
    setDarkMode(!darkMode);
    showToast('Theme updated successfully', 'success');
  };

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

  const renderSystemStatus = () => (
    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-6">
      <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-lg font-semibold text-gray-900 dark:text-white">Sources</h3>
            <p className="text-3xl font-bold text-blue-600 dark:text-blue-400">{systemStats.sources}</p>
          </div>
          <div className="text-4xl">??</div>
        </div>
      </div>
      
      <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-lg font-semibold text-gray-900 dark:text-white">Sinks</h3>
            <p className="text-3xl font-bold text-green-600 dark:text-green-400">{systemStats.sinks}</p>
          </div>
          <div className="text-4xl">??</div>
        </div>
      </div>
      
      <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-lg font-semibold text-gray-900 dark:text-white">Live Streams</h3>
            <p className="text-3xl font-bold text-purple-600 dark:text-purple-400">{systemStats.activeStreams}</p>
          </div>
          <div className="text-4xl">??</div>
        </div>
      </div>
      
      <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-lg font-semibold text-gray-900 dark:text-white">Server Status</h3>
            <p className={`text-3xl font-bold ${
              systemStats.serverStatus === 'online' ? 'text-green-600 dark:text-green-400' :
              systemStats.serverStatus === 'offline' ? 'text-gray-600 dark:text-gray-400' :
              'text-red-600 dark:text-red-400'
            }`}>
              {systemStats.serverStatus === 'online' ? 'Online' : 
               systemStats.serverStatus === 'offline' ? 'Offline' : 'Error'}
            </p>
          </div>
          <div className="text-4xl">
            {systemStats.serverStatus === 'online' ? '?' : 
             systemStats.serverStatus === 'offline' ? '?' : '?'}
          </div>
        </div>
      </div>
    </div>
  );

  const renderDashboard = () => (
    <div className="space-y-6">
      {renderSystemStatus()}

      {streamingSinks.size > 0 && (
        <div className="space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="text-xl font-bold text-gray-900 dark:text-white">Live Streams</h2>
            <button 
              onClick={() => setStreamingSinks(new Set())}
              className="px-3 py-1 bg-red-600 text-white rounded text-sm hover:bg-red-700"
            >
              Stop All Streams
            </button>
          </div>
          <div className="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-6">
            {Array.from(streamingSinks).map(sinkId => (
              <WebRTCStream
                key={sinkId}
                sinkId={sinkId}
                onStop={() => stopStream(sinkId)}
                onError={(error) => handleStreamError(sinkId, error)}
                className="transition-all hover:scale-105"
              />
            ))}
          </div>
        </div>
      )}

      {streamingSinks.size === 0 && (
        <div className="text-center py-12">
          <div className="text-6xl mb-4">??</div>
          <h3 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">No Active Streams</h3>
          <p className="text-gray-600 dark:text-gray-400 mb-4">Start streaming from the Sinks tab to see live video feeds here.</p>
          <button 
            onClick={() => setCurrentTab('sinks')}
            className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700"
          >
            Go to Sinks
          </button>
        </div>
      )}
    </div>
  );

  const renderSources = () => (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <div>
          <h2 className="text-2xl font-bold text-gray-900 dark:text-white">Video Sources</h2>
          <p className="text-gray-600 dark:text-gray-400">Manage your video input sources</p>
        </div>
        <button className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 transition-colors">
          + Add Source
        </button>
      </div>
      
      {sources.length === 0 && !loading ? (
        <div className="text-center py-12">
          <div className="text-6xl mb-4">??</div>
          <h3 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">No Sources Found</h3>
          <p className="text-gray-600 dark:text-gray-400">Add a video source to get started with processing.</p>
        </div>
      ) : (
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {sources.map(source => (
            <div key={source.id} className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg hover:scale-105">
              <div className="flex justify-between items-start mb-4">
                <h3 className="text-lg font-semibold text-gray-900 dark:text-white">{source.name}</h3>
                <span className={`px-2 py-1 rounded text-xs font-medium ${
                  source.status === 'active' ? 'bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200' :
                  source.status === 'inactive' ? 'bg-gray-100 text-gray-800 dark:bg-gray-700 dark:text-gray-300' :
                  'bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200'
                }`}>
                  {source.status}
                </span>
              </div>
              <p className="text-gray-600 dark:text-gray-400 mb-2">Type: {source.type}</p>
              <p className="text-sm text-gray-500 dark:text-gray-500 mb-4">ID: {source.id}</p>
              <div className="flex justify-between items-center">
                <span className="text-xs text-gray-400">
                  Updated: {source.lastUpdate?.toLocaleTimeString()}
                </span>
                <button className="px-3 py-1 bg-blue-600 text-white rounded text-sm hover:bg-blue-700">
                  Configure
                </button>
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );

  const renderSinks = () => (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <div>
          <h2 className="text-2xl font-bold text-gray-900 dark:text-white">Processing Sinks</h2>
          <p className="text-gray-600 dark:text-gray-400">Manage your vision processing pipelines</p>
        </div>
        <button className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 transition-colors">
          + Add Sink
        </button>
      </div>
      
      {sinks.length === 0 && !loading ? (
        <div className="text-center py-12">
          <div className="text-6xl mb-4">??</div>
          <h3 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">No Sinks Found</h3>
          <p className="text-gray-600 dark:text-gray-400">Add a processing sink to analyze video streams.</p>
        </div>
      ) : (
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {sinks.map(sink => (
            <div key={sink.id} className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg">
              <div className="flex justify-between items-start mb-4">
                <h3 className="text-lg font-semibold text-gray-900 dark:text-white">{sink.name}</h3>
                <div className="flex items-center gap-2">
                  {streamingSinks.has(sink.id) && (
                    <span className="px-2 py-1 bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200 rounded text-xs font-medium animate-pulse">
                      LIVE
                    </span>
                  )}
                  <span className={`px-2 py-1 rounded text-xs font-medium ${
                    sink.status === 'active' ? 'bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200' :
                    sink.status === 'inactive' ? 'bg-gray-100 text-gray-800 dark:bg-gray-700 dark:text-gray-300' :
                    'bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200'
                  }`}>
                    {sink.status}
                  </span>
                </div>
              </div>
              
              <p className="text-gray-600 dark:text-gray-400 mb-2">Type: {sink.type}</p>
              <p className="text-sm text-gray-500 dark:text-gray-500 mb-4">ID: {sink.id}</p>
              
              {sink.sourceId && (
                <p className="text-xs text-blue-600 dark:text-blue-400 mb-4">
                  Bound to Source {sink.sourceId}
                </p>
              )}
              
              <div className="flex gap-2 mb-4">
                {streamingSinks.has(sink.id) ? (
                  <button
                    onClick={() => stopStream(sink.id)}
                    className="flex-1 px-3 py-2 bg-red-600 text-white rounded text-sm hover:bg-red-700 transition-colors"
                  >
                    ?? Stop Stream
                  </button>
                ) : (
                  <button
                    onClick={() => startStream(sink.id)}
                    className="flex-1 px-3 py-2 bg-green-600 text-white rounded text-sm hover:bg-green-700 transition-colors"
                    disabled={sink.status === 'error'}
                  >
                    ?? Start Stream
                  </button>
                )}
                <button className="px-3 py-2 bg-gray-600 text-white rounded text-sm hover:bg-gray-700 transition-colors">
                  ??
                </button>
              </div>
              
              <div className="text-xs text-gray-400">
                Updated: {sink.lastUpdate?.toLocaleTimeString()}
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );

  if (loading) {
    return (
      <div className="min-h-screen bg-gray-50 dark:bg-gray-900 flex items-center justify-center">
        <div className="text-center">
          <div className="animate-spin rounded-full h-32 w-32 border-b-2 border-blue-600 mx-auto mb-4"></div>
          <p className="text-gray-600 dark:text-gray-400 text-lg">Loading FRCV Dashboard...</p>
          <p className="text-gray-500 dark:text-gray-500 text-sm mt-2">Connecting to {settings.serverUrl}</p>
        </div>
      </div>
    );
  }

  return (
    <div className={`min-h-screen transition-colors duration-200 ${darkMode ? 'dark' : ''}`}>
      <div className="min-h-screen bg-gray-50 dark:bg-gray-900">
        {/* Header */}
        <header className="bg-white dark:bg-gray-800 shadow-sm border-b border-gray-200 dark:border-gray-700">
          <div className="px-6 py-4">
            <div className="flex items-center justify-between">
              <div className="flex items-center space-x-4">
                <div className="flex items-center space-x-2">
                  <div className="text-2xl">??</div>
                  <h1 className="text-2xl font-bold text-gray-900 dark:text-white">FRCV Dashboard</h1>
                </div>
                <span className={`px-3 py-1 rounded-full text-sm font-medium ${
                  systemStats.serverStatus === 'online' 
                    ? 'bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200' 
                    : systemStats.serverStatus === 'error'
                    ? 'bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200'
                    : 'bg-gray-100 text-gray-800 dark:bg-gray-700 dark:text-gray-300'
                }`}>
                  {systemStats.serverStatus === 'online' ? '? Connected' : 
                   systemStats.serverStatus === 'error' ? '? Error' : '? Disconnected'}
                </span>
              </div>
              
              <div className="flex items-center space-x-4">
                <button
                  onClick={() => setShowSettings(true)}
                  className="p-2 rounded-lg bg-gray-100 dark:bg-gray-700 text-gray-600 dark:text-gray-300 hover:bg-gray-200 dark:hover:bg-gray-600 transition-colors"
                  title="Settings"
                >
                  ??
                </button>
                
                <button
                  onClick={toggleDarkMode}
                  className="p-2 rounded-lg bg-gray-100 dark:bg-gray-700 text-gray-600 dark:text-gray-300 hover:bg-gray-200 dark:hover:bg-gray-600 transition-colors"
                  title="Toggle dark mode"
                >
                  {darkMode ? '??' : '??'}
                </button>
                
                <button
                  onClick={loadData}
                  className="px-3 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 transition-colors"
                  title="Refresh data"
                >
                  ?? Refresh
                </button>
              </div>
            </div>
          </div>
        </header>

        {/* Navigation */}
        <nav className="bg-white dark:bg-gray-800 border-b border-gray-200 dark:border-gray-700">
          <div className="px-6">
            <div className="flex space-x-8">
              {[
                { id: 'dashboard', label: 'Dashboard', icon: '??' },
                { id: 'sources', label: 'Sources', icon: '??' },
                { id: 'sinks', label: 'Sinks', icon: '??' },
              ].map(tab => (
                <button
                  key={tab.id}
                  onClick={() => setCurrentTab(tab.id)}
                  className={`flex items-center space-x-2 py-4 px-2 border-b-2 font-medium text-sm transition-colors ${
                    currentTab === tab.id
                      ? 'border-blue-500 text-blue-600 dark:text-blue-400'
                      : 'border-transparent text-gray-500 hover:text-gray-700 dark:text-gray-400 dark:hover:text-gray-300'
                  }`}
                >
                  <span>{tab.icon}</span>
                  <span>{tab.label}</span>
                  {tab.id === 'dashboard' && streamingSinks.size > 0 && (
                    <span className="bg-red-500 text-white text-xs rounded-full px-2 py-0.5 min-w-[20px] h-5 flex items-center justify-center">
                      {streamingSinks.size}
                    </span>
                  )}
                </button>
              ))}
            </div>
          </div>
        </nav>

        {/* Error Banner */}
        {error && (
          <div className="bg-red-100 dark:bg-red-900 border-l-4 border-red-500 p-4 mx-6 mt-4 rounded">
            <div className="flex">
              <div className="flex-shrink-0">
                <span className="text-red-500">??</span>
              </div>
              <div className="ml-3">
                <p className="text-sm text-red-700 dark:text-red-200">
                  Connection Error: {error}
                </p>
              </div>
              <div className="ml-auto pl-3">
                <button
                  onClick={() => setError(null)}
                  className="text-red-500 hover:text-red-700"
                >
                  ?
                </button>
              </div>
            </div>
          </div>
        )}

        {/* Main Content */}
        <main className="p-6">
          {currentTab === 'dashboard' && renderDashboard()}
          {currentTab === 'sources' && renderSources()}
          {currentTab === 'sinks' && renderSinks()}
        </main>

        {/* Settings Modal */}
        <SettingsModal
          isOpen={showSettings}
          onClose={() => setShowSettings(false)}
          settings={settings}
          onSave={setSettings}
        />

        {/* Toast Notifications */}
        {toast && (
          <Toast
            message={toast.message}
            type={toast.type}
            onClose={() => setToast(null)}
          />
        )}
      </div>
    </div>
  );
}

export default App;
