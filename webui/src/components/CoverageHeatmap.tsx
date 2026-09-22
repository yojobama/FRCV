import React, { useEffect, useRef } from 'react';
import type { CalibrationCoverage } from '../types';

// ROADMAP.md Phase 8d: a live coverage heatmap during the calibration wizard's capture step -
// "which region of the frame still needs more checkerboard coverage", the gap the plan calls out
// as "the biggest usability gap in every existing FRC calibration tool". Plots every saved
// snapshot's detected corner points (each already flattened [x0,y0,x1,y1,...] by the server -
// see CalibrationCoverageDto's own comment on why) over the frame bounds; a region with no dots
// near it has never been covered by a checkerboard yet.
export const CoverageHeatmap: React.FC<{ coverage: CalibrationCoverage | null; className?: string }> = ({ coverage, className = '' }) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const width = canvas.width;
    const height = canvas.height;
    ctx.clearRect(0, 0, width, height);

    // background + frame border
    ctx.fillStyle = 'rgba(107, 114, 128, 0.15)'; // gray-500 @ 15%
    ctx.fillRect(0, 0, width, height);
    ctx.strokeStyle = 'rgba(107, 114, 128, 0.5)';
    ctx.strokeRect(0.5, 0.5, width - 1, height - 1);

    if (!coverage || coverage.FrameWidth <= 0 || coverage.FrameHeight <= 0) return;

    const scaleX = width / coverage.FrameWidth;
    const scaleY = height / coverage.FrameHeight;

    // each snapshot's own corner set drawn as a connected outline (its convex-hull-ish
    // perimeter would be more precise, but the raw detection order already traces the board's
    // own grid boundary closely enough to read as "this region was covered") plus dots at each
    // corner, so both the covered outline and the corner density are visible at a glance.
    coverage.Snapshots.forEach((flat, i) => {
      if (flat.length < 4) return;
      const hue = (i * 47) % 360; // spread distinct snapshots across the color wheel
      ctx.strokeStyle = `hsla(${hue}, 70%, 55%, 0.5)`;
      ctx.fillStyle = `hsla(${hue}, 70%, 55%, 0.7)`;
      ctx.lineWidth = 1;

      ctx.beginPath();
      for (let p = 0; p + 1 < flat.length; p += 2) {
        const x = flat[p] * scaleX;
        const y = flat[p + 1] * scaleY;
        if (p === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.closePath();
      ctx.stroke();

      for (let p = 0; p + 1 < flat.length; p += 2) {
        const x = flat[p] * scaleX;
        const y = flat[p + 1] * scaleY;
        ctx.beginPath();
        ctx.arc(x, y, 2, 0, Math.PI * 2);
        ctx.fill();
      }
    });
  }, [coverage]);

  return (
    <div className={className}>
      <canvas ref={canvasRef} width={480} height={360} className="w-full h-auto bg-gray-100 dark:bg-gray-900 rounded border border-gray-300 dark:border-gray-600" />
      <p className="text-xs text-gray-500 dark:text-gray-400 mt-1">
        {coverage ? `${coverage.Snapshots.length} snapshot${coverage.Snapshots.length === 1 ? '' : 's'} · ${coverage.FrameWidth}×${coverage.FrameHeight}` : 'No coverage data yet'}
      </p>
    </div>
  );
};
