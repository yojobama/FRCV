import { useEffect, useRef, useState } from 'react';
import type { StateSnapshot } from '../types';

// ROADMAP.md Phase 8a/8c: consumes the /ws/state push channel (Server/WebSockets/StateChannel.cs)
// that replaces the old per-client polling loop useAppData.ts still uses. This is the graph
// page's own data source - one consolidated snapshot per server tick, not four+N HTTP requests
// per client per refresh.
export function useStateSocket() {
  const [snapshot, setSnapshot] = useState<StateSnapshot | null>(null);
  const [connected, setConnected] = useState(false);
  const socketRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    let cancelled = false;
    let reconnectTimer: ReturnType<typeof setTimeout> | undefined;

    const connect = () => {
      if (cancelled) return;
      // ws:// mirrors whatever protocol the page itself was served over (wss:// for an https
      // deployment) - same reasoning as ApiService's own window.location.origin-relative baseUrl.
      const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      const socket = new WebSocket(`${protocol}//${window.location.host}/ws/state`);
      socketRef.current = socket;

      socket.onopen = () => setConnected(true);
      socket.onmessage = (event) => {
        try {
          setSnapshot(JSON.parse(event.data));
        } catch (err) {
          console.warn('useStateSocket: failed to parse snapshot', err);
        }
      };
      socket.onclose = () => {
        setConnected(false);
        if (!cancelled) reconnectTimer = setTimeout(connect, 2000);
      };
      socket.onerror = () => socket.close();
    };

    connect();

    return () => {
      cancelled = true;
      clearTimeout(reconnectTimer);
      socketRef.current?.close();
    };
  }, []);

  return { snapshot, connected };
}
