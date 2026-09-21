import React from 'react';
import { Gauge } from 'lucide-react';

// ROADMAP.md Phase 8e: read-only per-camera FPS/latency/NT4-state/temperature, readable across
// a pit. Placeholder route for now (Phase 8b's own verification bar is that /match is reachable)
// - the real content depends on the /ws/state channel's per-node stats (Phase 8a, already
// shipped) once the frontend has a WebSocket hook to consume it (Phase 8c/8e).
export const MatchPage: React.FC = () => (
  <div className="flex flex-col items-center justify-center py-24 text-center">
    <Gauge className="w-16 h-16 mb-4 text-gray-400" />
    <h2 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">Match View</h2>
    <p className="text-gray-600 dark:text-gray-400 max-w-md">
      A read-only per-camera FPS/latency/NT4/temperature view lands in ROADMAP.md Phase 8e.
    </p>
  </div>
);
