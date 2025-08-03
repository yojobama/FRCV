import React, { useState, useEffect, useRef } from 'react';
import {
  RefreshCw,
  Minimize2,
  Maximize2,
  Square,
  Video,
  Activity,
  XCircle
} from 'lucide-react';
import { WebRTCStreamProps } from '../types';
import { ApiService } from '../services/ApiService';

export const WebRTCStream: React.FC<WebRTCStreamProps> = ({ 
  sinkId, 
  onStop, 
  onError, 
  className = '' 
}) => {
  const [connectionState, setConnectionState] = useState<string>('connecting');
  const [peerConnection, setPeerConnection] = useState<RTCPeerConnection | null>(null);
  const [isFullscreen, setIsFullscreen] = useState(false);
  const videoRef = useRef<HTMLVideoElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
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
          <Video className="w-5 h-5 text-white" />
          <span className="text-white font-medium">Sink {sinkId} Stream</span>
          <span className={`px-2 py-1 rounded text-xs text-white ${getStatusColor(connectionState)} animate-pulse-slow flex items-center gap-1`}>
            <Activity className="w-3 h-3" />
            {getStatusText(connectionState)}
          </span>
        </div>
        
        <div className="flex items-center gap-2">
          <button
            onClick={toggleFullscreen}
            className="px-3 py-1 bg-blue-600 text-white rounded text-sm hover:bg-blue-700 transition-colors flex items-center gap-1"
            title="Toggle fullscreen"
          >
            {isFullscreen ? <Minimize2 className="w-3 h-3" /> : <Maximize2 className="w-3 h-3" />}
          </button>
          <button
            onClick={stopStream}
            className="px-3 py-1 bg-red-600 text-white rounded text-sm hover:bg-red-700 transition-colors flex items-center gap-1"
            title="Stop stream"
          >
            <Square className="w-3 h-3" />
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
              <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-white mx-auto mb-2">
                <RefreshCw className="w-8 h-8" />
              </div>
              <p className="text-sm">Connecting to stream...</p>
            </div>
          </div>
        )}
        
        {connectionState === 'failed' && (
          <div className="absolute inset-0 flex items-center justify-center bg-red-900 bg-opacity-75">
            <div className="text-center text-white">
              <div className="flex items-center justify-center gap-2 mb-2">
                <XCircle className="w-5 h-5" />
                <p className="text-sm">Connection failed</p>
              </div>
              <button 
                onClick={startWebRTCConnection}
                className="mt-2 px-3 py-1 bg-white text-red-900 rounded text-sm hover:bg-gray-100 flex items-center gap-1"
              >
                <RefreshCw className="w-3 h-3" />
                Retry
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};