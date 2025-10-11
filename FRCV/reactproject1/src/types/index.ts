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

export interface AddSinkModalProps extends ModalProps {
  onAdd: (name: string, type: string) => void;
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