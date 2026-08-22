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