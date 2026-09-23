import React, { useCallback, useEffect, useRef, useState } from 'react';
import { RefreshCw } from 'lucide-react';
import { WebRTCStream } from './WebRTCStream';
import { MjpegStream } from './MjpegStream';
import { ApiService } from '../services/ApiService';
import type { WebRTCStreamProps } from '../types';

const api = new ApiService();

interface StreamViewProps extends WebRTCStreamProps {
  // the node whose output the live sink should bind to - required to spin up the MJPEG fallback
  // sink lazily (bound to the SAME source the failed WebRTC sink was reading from). Every call
  // site already has this on hand (it's what it bound the WebRTC sink to in the first place); a
  // null here just means "no fallback possible", not an error - the WebRTC failure is still
  // surfaced via onError as before.
  sourceId: number | null;
}

type FallbackState = { kind: 'creating' } | { kind: 'ready'; sinkId: number } | { kind: 'failed' };

// WebRTC is the standard transport for every live preview here - lower latency, no per-frame
// HTTP/JPEG framing overhead, and it's what every call site already used before this component
// existed. MJPEG is not a parallel always-on alternative (that would mean encoding every preview
// twice for no reason): it exists purely as a same-preview fallback for the one failure mode
// WebRTC can't route around - ICE/STUN/SDP negotiation breaking outright, the documented reason
// this sink exists at all (docs/history/IMPLEMENTATION_PLAN.md Phase 6 item 5) - typically a
// locked-down or unfamiliar competition network. Spun up reactively, on failure, and logged when
// it happens so a fallback in the field is diagnosable after the fact, not silent.
export const StreamView: React.FC<StreamViewProps> = ({ sourceId, onError, sinkId, ...rest }) => {
  const [fallback, setFallback] = useState<FallbackState | null>(null);
  // WebRTCStream's pc.onconnectionstatechange can call onError more than once in quick
  // succession (e.g. 'disconnected' immediately followed by 'failed') - confirmed the hard way:
  // checking React state alone let two near-simultaneous calls both read the same stale
  // `fallback === null` before either one's setFallback({kind:'creating'}) had actually
  // committed a re-render, so both proceeded to create their own MjpegSink, leaking one. A ref
  // is checked and set synchronously, before any await, so the second call sees it immediately
  // regardless of React's render timing.
  const fallbackStarted = useRef(false);

  useEffect(() => {
    fallbackStarted.current = false;
    setFallback(null);
  }, [sinkId]);

  const handleWebRtcError = useCallback(async (error: string) => {
    if (sourceId == null || fallbackStarted.current) {
      // no source to bind a fallback sink to, or a fallback attempt is already in flight/done
      // (this is either a duplicate WebRTC failure notification, or the MJPEG sink's own failure
      // bubbling back up) - just surface it.
      onError(error);
      return;
    }
    fallbackStarted.current = true;
    console.warn('StreamView: WebRTC preview failed, falling back to MJPEG', { sinkId, sourceId, error });
    setFallback({ kind: 'creating' });
    try {
      const mjpegId = await api.createMjpegSink(`preview-mjpeg-${sourceId}`);
      await api.bindSinkToSource(mjpegId, sourceId);
      await api.toggleSink(mjpegId, true);
      setFallback({ kind: 'ready', sinkId: mjpegId });
    } catch (fallbackError) {
      console.error('StreamView: MJPEG fallback failed to start', fallbackError);
      setFallback({ kind: 'failed' });
      onError(error);
    }
  }, [sourceId, onError, sinkId]);

  if (fallback?.kind === 'ready') {
    return <MjpegStream sinkId={fallback.sinkId} onError={onError} {...rest} />;
  }

  if (fallback?.kind === 'creating') {
    return (
      <div className={`bg-black rounded-lg overflow-hidden flex items-center justify-center ${rest.compact ? 'h-full' : 'h-64'} ${rest.className ?? ''}`}>
        <div className="text-center text-white">
          <RefreshCw className="w-6 h-6 mx-auto mb-2 animate-spin" />
          <p className="text-sm">WebRTC failed - starting MJPEG fallback...</p>
        </div>
      </div>
    );
  }

  return <WebRTCStream sinkId={sinkId} onError={handleWebRtcError} {...rest} />;
};
