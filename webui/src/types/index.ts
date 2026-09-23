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

// mirrors Server/Dtos.cs's CameraHardwareInfoDto - PascalCase, see the note on
// CameraCalibrationResult below. Was declared lowercase here until caught live: the "Camera
// Device" dropdown in AddSourceModal rendered every option as the literal text "()" (camera.name/
// camera.path both undefined), and submitting silently POSTed an empty {} body to
// /cameraSource/create - found while testing camera creation against a real USB camera on the
// Orange Pi.
export interface CameraHardwareInfo {
  Name: string;
  Path: string;
}

// mirrors Server/Dtos.cs's CameraModeDto - PascalCase, see CameraCalibrationResult's note below.
// PixelFormat is FrameFormat's ordinal (LumenCore/FrameFormat.h): 0 BGR24, 1 RGB24, 2 GRAY8,
// 3 NV12, 4 YUYV, 5 MJPEG.
export interface CameraMode {
  Width: number;
  Height: number;
  Fps: number;
  PixelFormat: number;
  IsNative: boolean;
}

// mirrors Server/Dtos.cs's CalibrationStatusDto (ROADMAP.md Phase 8/E5) - whether a camera
// source has a saved calibration at all, and whether it still matches the camera's CURRENT
// capture mode (a SetMode call can silently leave it stale).
export interface CalibrationStatus {
  HasCalibration: boolean;
  MatchesCurrentResolution: boolean;
  CalibratedWidth: number | null;
  CalibratedHeight: number | null;
}

// An uploaded ONNX object detection model (YOLOv8/YOLOv11), as returned by /model/getAll
export interface Model {
  id: number;
  name: string;
  variant: number; // 0 = YOLOv8, 1 = YOLOv11
  inputSize: number;
  confThreshold: number;
  nmsThreshold: number;
  // which backend this model runs on - derived from the uploaded file's own extension at
  // upload time (ModelManager.AddModel), not a separate preference: 0 = RKNN (NPU), 1 = ONNX
  // Runtime (matches LumenCore/Manager.h's ObjectDetectionProvider declaration order).
  provider: number;
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

// New types for device monitoring - sourced from /ws/state's WsDeviceStats (useAppData.ts),
// not a separate REST poll. ramUsage is megabytes, not bytes - WsDeviceStats.RamUsageMb already
// reports it that way (unlike the old getDeviceRAMUsage() REST call, which returned raw bytes).
export interface DeviceStats {
  cpuUsage: number;
  ramUsage: number;
  diskUsage: number;
}

// --- Stereo depth (phase 10) - see STEREO_IMPLEMENTATION_PLAN.md ---

// ROADMAP.md Phase 8a/8d: mirrors Server/Dtos.cs's CameraCalibrationResultDto/
// StereoCalibrationResultDto field-for-field, PascalCase and all - confirmed empirically (see
// the note by PipelineProfile above). These two interfaces were previously lowercase-first,
// matching the RAW SWIG-serialized shape the endpoints returned before the Phase 8a DTO
// cleanup - a real regression this introduced and StereoPage.tsx was silently broken by
// (result.epipolarRms read as undefined) until caught and fixed here.
export interface CameraCalibrationResult {
  Fx: number; Fy: number; Cx: number; Cy: number; Rms: number;
  DistCoeffs: number[]; ImageWidth: number; ImageHeight: number;
}

// R/T/E/F/R1/R2/P1/P2/Q are flat row-major arrays (see StereoCalibrationResult.h's own comments
// for each matrix's shape) - only EpipolarRms, BaselineMeters, RectifiedFx/Cx/Cy and the two
// CameraCalibrationResults are actually read by this WebUI; the rest is carried through opaquely
// to StereoDepthSink's create call.
// the real gate for real use, not stereoRms - see STEREO_IMPLEMENTATION_PLAN.md ss10.2. Shared
// between StereoPage.tsx and the Phase 8d calibration wizards, which both need the same number.
export const EPIPOLAR_RMS_GATE = 0.5;

export interface StereoCalibrationResult {
  Left: CameraCalibrationResult;
  Right: CameraCalibrationResult;
  R: number[]; T: number[]; E: number[]; F: number[];
  R1: number[]; R2: number[]; P1: number[]; P2: number[]; Q: number[];
  StereoRms: number;
  EpipolarRms: number; // the real gate for real use - see STEREO_IMPLEMENTATION_PLAN.md ss10.2. < 0.5px
  BaselineMeters: number;
  RectifiedFx: number; RectifiedCx: number; RectifiedCy: number;
  ImageWidth: number; ImageHeight: number;
  RoiLeftX: number; RoiLeftY: number; RoiLeftW: number; RoiLeftH: number;
  RoiRightX: number; RoiRightY: number; RoiRightW: number; RoiRightH: number;
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

// mirrors Server/Dtos.cs's CalibrationCoverageDto - ROADMAP.md Phase 8d's live coverage
// heatmap. Each entry in Snapshots is one saved snapshot/pair's detected corners flattened as
// [x0,y0,x1,y1,...] (see that DTO's own comment on why - no vector<vector<double>> SWIG binding).
export interface CalibrationCoverage {
  FrameWidth: number;
  FrameHeight: number;
  Snapshots: number[][];
}

// mirrors Server/Dtos.cs's StereoDepthStatsDto - PascalCase, see the note above.
export interface StereoDepthStats {
  ValidFraction: number;
  MedianDepthMeters: number;
}

// --- ROADMAP.md Phase 7/8: pipeline profiles + the /ws/state channel + node capabilities ---
// These mirror the real C# response shapes field-for-field, PascalCase and all - confirmed
// empirically (Server/Dtos.cs and friends are serialized by EmbedIO's own Swan formatter, which
// (unlike System.Text.Json defaults elsewhere) was checked directly against a live server and
// does NOT camelCase or otherwise rename properties) rather than assumed, the same way the
// stereo types above document doing.

// mirrors Server/PipelineProfile.cs
export const PipelineProfileKind = { ApriltagSink: 0, ObjectDetectionSink: 1 } as const;
export interface PipelineProfile {
  Index: number;
  Name: string;
  Kind: number;
  TagSize: number | null;
  CalibratorSinkId: number | null;
  Backend: number | null;
  FrameWidth: number;
  FrameHeight: number;
  FieldLayoutPath: string | null;
  DriverMode: boolean;
  ModelId: number | null;
}

// mirrors Server/NodeCapabilities.cs
export interface NodeTypeCapability {
  TypeName: string;
  Category: 'source' | 'sink';
  DisplayName: string;
  Icon: string;
  MaxSources: number;
  SourceRoles: string[] | null;
  IsDualRoleSink: boolean;
  HasDepthAttach: boolean;
  Implemented: boolean;
}
export interface NodeTypesResponse {
  Sources: NodeTypeCapability[];
  Sinks: NodeTypeCapability[];
}

// mirrors Server/Source.cs, as embedded in the /ws/state channel and Sink.Source/Source2.
// CameraHardwareInfo here is lowercase {name,path} - unlike every other field on this type, it's
// the raw native SWIG CameraHardwareInfo passed straight through (Source.cs's own property is a
// CameraHardwareInfo, not a DTO wrapper), and SWIG generated lowercase C# properties for it since
// that's what the C++ struct's own members are named. Confirmed empirically against a live
// /ws/state frame: {"CameraHardwareInfo":{"name":"...","path":"/dev/video2"},...} - do NOT
// "fix" this to PascalCase to match the rest of the file, that would silently break it again.
// (CameraHardwareInfo below, the OTHER one, IS PascalCase - it comes from CameraHardwareInfoDto,
// a real DTO wrapper used by /cameraSource/getNotRegistered and /cameraSource/create.)
export interface WsSource {
  CameraHardwareInfo: { name: string; path: string } | null;
  Fps: number | null;
  FilePath: string | null;
  Profiles: PipelineProfile[];
  ActiveProfileIndex: number;
  ActiveDetectionSinkId: number | null;
  Type: number; // 0 Camera, 1 ImageFile, 2 VideoFile, 3 SinkOutput - see SourceType in Source.cs
  Id: number;
  Name: string;
}

// mirrors Server/Sink.cs
export interface WsSink {
  Type: number; // SinkType ordinal - see mapSinkType in hooks/useAppData.ts for the string labels
  Id: number;
  Name: string;
  Source: WsSource | null;
  Source2: WsSource | null; // stereo sinks only - the RIGHT camera (Source is LEFT)
  DepthSourceId: number | null; // DepthFusionSink only
}

// mirrors Server/WebSockets/StateChannel.cs's own DTOs
export interface WsSinkState {
  Sink: WsSink;
  IsRunning: boolean;
}
export interface WsNodeStats {
  Fps: number;
  LatencyUs: number;
}
export interface WsDeviceStats {
  CpuUsagePercent: number;
  RamUsageMb: number;
  DiskUsagePercent: number;
  TemperatureC: number;
}
// mirrors Server/Dtos.cs's NetworkTablesStatusDto - ROADMAP.md Phase 8e's match view reads
// Connected off this for each NetworkTablesSink it finds bound to a camera's detection chain.
export interface NetworkTablesStatus {
  Connected: boolean;
  Identity: string;
  RootTable: string;
  TeamNumber: number | null;
  ServerAddress: string | null;
}

export interface StateSnapshot {
  Sources: WsSource[];
  Sinks: WsSinkState[];
  Device: WsDeviceStats;
  NodeStats: Record<string, WsNodeStats>;
}