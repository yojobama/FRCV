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
  className = '',
  compact = false,
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

      // No public STUN server by default - this stream is between the browser and a coprocessor
      // on the same LAN (a pit/venue network), which doesn't need NAT traversal, and "fully
      // offline, venue WiFi is hostile" (ROADMAP.md Phase 8) rules out depending on reaching the
      // public internet for every preview to connect. iceServers stays empty rather than
      // hardcoded to a specific public server; a future settings-driven override can populate it
      // for the rare cross-network setup that genuinely needs one.
      const config: RTCConfiguration = { iceServers: [] };

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

      // WebRTCSink uses non-trickle ICE on LumenVision's side (see WebRTCSinkController) - it gathers
      // its own candidates internally before ever returning an offer. The browser's candidates
      // still trickle one at a time here, starting as soon as setLocalDescription() is called
      // below - which is BEFORE the answer has been POSTed to the server. Sending a candidate
      // ahead of the answer isn't just late, it crashes the whole server process: libdatachannel
      // throws if a remote candidate arrives before the remote description is set, and that
      // exception was crossing the P/Invoke boundary uncaught (confirmed the hard way - fixed
      // server-side too in WebRTCSink::AddIceCandidate, but there's no reason to rely on that as
      // the only guard). Buffer every candidate here and only flush them once the answer POST
      // has actually completed.
      let answerSent = false;
      const pendingCandidates: RTCIceCandidate[] = [];
      pc.onicecandidate = async (event) => {
        if (!event.candidate) return;
        if (!answerSent) {
          pendingCandidates.push(event.candidate);
          return;
        }
        try {
          await api.sendWebRTCIceCandidate(sinkId, event.candidate.candidate, event.candidate.sdpMid || '');
        } catch (err) {
          console.warn('Failed to send ICE candidate:', err);
        }
      };

      // getWebRTCOffer blocks briefly server-side for its own ICE gathering, then returns a
      // plain SDP string (not a JSON envelope) - see WebRTCSinkController.CreateOffer.
      const offerSdp = await api.getWebRTCOffer(sinkId);
      await pc.setRemoteDescription({ type: 'offer', sdp: offerSdp });
      const answer = await pc.createAnswer();
      await pc.setLocalDescription(answer);
      await api.sendWebRTCAnswer(sinkId, answer.sdp || '');
      answerSent = true;
      for (const candidate of pendingCandidates) {
        try {
          await api.sendWebRTCIceCandidate(sinkId, candidate.candidate, candidate.sdpMid || '');
        } catch (err) {
          console.warn('Failed to send buffered ICE candidate:', err);
        }
      }
    } catch (error) {
      console.error('WebRTC connection failed:', error);
      setConnectionState('failed');
      onError(error instanceof Error ? error.message : 'Connection failed');
    }
  };

  const stopStream = () => {
    // No server-side "stop" endpoint exists for a WebRTCSink (see WebRTCSinkController) -
    // closing the local RTCPeerConnection is all a viewer needs to do; the sink itself keeps
    // running until its own toggle/delete is used.
    if (peerConnection) {
      peerConnection.close();
    }
    onStop();
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

  const getStatuscolour = (state: string) => {
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
    <div ref={containerRef} className={`bg-black rounded-lg overflow-hidden ${compact ? 'h-full' : ''} ${className}`}>
      {!compact && (
      <div className="p-4 bg-gray-800 flex justify-between items-center">
        <div className="flex items-center space-x-3">
          <Video className="w-5 h-5 text-white" />
          <span className="text-white font-medium">Sink {sinkId} Stream</span>
          <span className={`px-2 py-1 rounded text-xs text-white ${getStatuscolour(connectionState)} animate-pulse-slow flex items-center gap-1`}>
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
      )}

      <div className={compact ? 'relative h-full' : 'relative'}>
        <video
          ref={videoRef}
          autoPlay
          muted
          playsInline
          className={compact ? 'w-full h-full object-cover bg-black' : 'w-full h-64 object-cover bg-black'}
          style={compact ? undefined : { aspectRatio: '16/9' }}
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