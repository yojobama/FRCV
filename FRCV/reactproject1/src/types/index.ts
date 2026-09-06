// Types
export interface Source {
  id: number;
  name: string;
  type: string;
  status: 'active' | 'inactive' | 'error';
  lastUpdate?: Date;
  filePath?: string; // For video/image file sources
  fps?: number; // For video file sources
  cameraHardwareInfo?: CameraHardwareInfo; // For camera sources
}

export interface Sink {
  id: number;
  name: string;
  type: string;
  status: 'active' | 'inactive' | 'error';
  isStreaming?: boolean;
  lastUpdate?: Date;
  sourceId?: number;
  // stereo sinks only (StereoCalibrationSink/StereoDepthSink) - sourceId is the LEFT camera,
  // this is the RIGHT one. See Sink.Source2 (Server/Sink.cs).
  source2Id?: number;
  isEnabled?: boolean; // Track whether sink is enabled/disabled
}

export interface CameraHardwareInfo {
  name: string;
  path: string;
}

// An uploaded ONNX object detection model (YOLOv8/YOLOv11), as returned by /model/getAll
export interface Model {
  id: number;
  name: string;
  variant: number; // 0 = YOLOv8, 1 = YOLOv11
  inputSize: number;
  confThreshold: number;
  nmsThreshold: number;
}

// Extra fields AddSinkModal collects for sink types that need more than just a name -
// handleAddSink dispatches on `type` to decide which of these actually apply. WebRTC and
// NetworkTables are no longer creatable this way (see AddSinkOptions vs. the per-node preview/
// publish toggles in useAppData.ts) - tagSize/backend cover AprilTag instead.
export interface AddSinkOptions {
  tagSize?: number;
  backend?: number; // 0 = CPU, 1 = Vulkan
  modelId?: number;
  newModel?: {
    name: string;
    variant: number;
    inputSize: number;
    confThreshold: number;
    nmsThreshold: number;
    modelFile: File;
    labelsFile?: File;
  };
}

// Shared connection defaults for the per-node "Publish to NetworkTables" toggle - configured
// once in Settings rather than re-entered every time, since a real robot only has one NT4
// server to talk to. Each toggle still creates its own dedicated NetworkTablesSink (ISink only
// binds one source at a time), just reusing these connection details.
export interface NT4Defaults {
  mode: 'team' | 'server';
  teamNumber?: number;
  serverAddress?: string;
  port?: number;
  rootTable: string;
}

export interface SystemStats {
  sources: number;
  sinks: number;
  activeStreams: number;
  uptime: string;
  serverStatus: 'online' | 'offline' | 'error';
  cpuUsage?: number;
  ramUsage?: number;
  diskUsage?: number;
}

export interface Settings {
  serverUrl: string;
  refreshInterval: number;
  nt4: NT4Defaults;
}

export interface Toast {
  message: string;
  type: 'success' | 'error' | 'info';
}

export interface WebRTCStreamProps {
  sinkId: number;
  onStop: () => void;
  onError: (error: string) => void;
  className?: string;
}

export interface ModalProps {
  isOpen: boolean;
  onClose: () => void;
}

export interface AddSourceModalProps extends ModalProps {
  onAdd: (name: string, type: string, file?: File, fps?: number, hardwareInfo?: CameraHardwareInfo) => void;
}

export interface SettingsModalProps extends ModalProps {
  settings: Settings;
  onSave: (settings: Settings) => void;
}

export interface ToastProps {
  message: string;
  type: 'success' | 'error' | 'info';
  onClose: () => void;
}

// New types for device monitoring
export interface DeviceStats {
  cpuUsage: number;
  ramUsage: number;
  diskUsage: number;
}

// --- Stereo depth (phase 10) - see STEREO_IMPLEMENTATION_PLAN.md ---

export interface CameraCalibrationResult {
  fx: number; fy: number; cx: number; cy: number; rms: number;
  distCoeffs: number[]; imageWidth: number; imageHeight: number;
}

// Mirrors StereoCalibrationResult.h field-for-field. R/T/E/F/R1/R2/P1/P2/Q are flat row-major
// arrays (see that header's own comments for each matrix's shape) - only epipolarRms,
// baselineMeters, rectifiedFx/Cx/Cy and the two CameraCalibrationResults are actually read by
// this WebUI; the rest is carried through opaquely to StereoDepthSink's create call.
export interface StereoCalibrationResult {
  left: CameraCalibrationResult;
  right: CameraCalibrationResult;
  R: number[]; T: number[]; E: number[]; F: number[];
  R1: number[]; R2: number[]; P1: number[]; P2: number[]; Q: number[];
  stereoRms: number;
  epipolarRms: number; // the real gate for real use - see STEREO_IMPLEMENTATION_PLAN.md ss10.2. < 0.5px
  baselineMeters: number;
  rectifiedFx: number; rectifiedCx: number; rectifiedCy: number;
  imageWidth: number; imageHeight: number;
  roiLeftX: number; roiLeftY: number; roiLeftW: number; roiLeftH: number;
  roiRightX: number; roiRightY: number; roiRightW: number; roiRightH: number;
}

// matches StereoDepthBackendKind.h - a plain C++ enum, so the values below are its declaration
// order (0-indexed), exactly what SWIG/System.Text.Json serialize an enum as.
export const StereoDepthBackendKind = {
  CODEC_AUTO: 0,
  CODEC_LAVC: 1,
  CODEC_RKMPP_HWENC: 2,
  SGBM: 3,
} as const;
export type StereoDepthBackendKindValue = typeof StereoDepthBackendKind[keyof typeof StereoDepthBackendKind];

export const STEREO_BACKEND_LABELS: Record<number, string> = {
  0: 'Auto (codec-stereo)',
  1: 'codec-stereo: lavc_sw (software, any platform)',
  2: 'codec-stereo: rkmpp_hwenc (Orange Pi hardware)',
  3: 'SGBM (OpenCV, CPU - accuracy reference)',
};

// matches StereoFrameOutput.h
export const StereoFrameOutput = {
  DEPTH_COLORMAP: 0,
  RECTIFIED_LEFT: 1,
  DEPTH_OVERLAY: 2,
} as const;
export type StereoFrameOutputValue = typeof StereoFrameOutput[keyof typeof StereoFrameOutput];

export const STEREO_FRAME_OUTPUT_LABELS: Record<number, string> = {
  0: 'Depth colormap',
  1: 'Rectified left (bind a detector here for DepthFusionNode)',
  2: 'Depth overlay',
};

export interface StereoDepthStats {
  validFraction: number;
  medianDepthMeters: number;
}