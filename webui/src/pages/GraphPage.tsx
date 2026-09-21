import React from 'react';
import { Workflow } from 'lucide-react';

// ROADMAP.md Phase 8c: the pipeline graph - a canvas-based node editor replacing the Sources/
// Sinks list pages once it reaches parity with them (create/configure/bind/delete for every
// node type). Placeholder route for now so /graph is a real, reachable page (Phase 8b's own
// verification bar) before Phase 8c builds the actual canvas.
export const GraphPage: React.FC = () => (
  <div className="flex flex-col items-center justify-center py-24 text-center">
    <Workflow className="w-16 h-16 mb-4 text-gray-400" />
    <h2 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">Pipeline Graph</h2>
    <p className="text-gray-600 dark:text-gray-400 max-w-md">
      The node-graph editor lands in ROADMAP.md Phase 8c. Until then, manage sources and sinks
      from their own pages.
    </p>
  </div>
);
