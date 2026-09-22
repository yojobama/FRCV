import type { CameraHardwareInfo, CameraMode, Model, StereoCalibrationResult, StereoDepthStats, PipelineProfile, NodeTypesResponse, CameraCalibrationResult, CalibrationCoverage, NetworkTablesStatus } from '../types';

export class ApiService {
  // Relative to wherever this page is served from - the C# server always serves its own built
  // WebUI, so hardcoding localhost:8175 broke it for anyone reaching the server by its real
  // hostname/IP (e.g. the Orange Pi over the network) rather than from a dev machine.
  private baseUrl = `${window.location.origin}/api`;

  // Source Controller routes
  async getSources(): Promise<any[]> {
    const response = await fetch(`${this.baseUrl}/source/getAll`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async deleteSource(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/source/delete?SourceID=${id}`, { method: 'DELETE' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async renameSource(id: number, name: string): Promise<void> {
    // Note: The route parameter name in SourceController appears to be SinkID (might be a copy-paste error)
    const response = await fetch(`${this.baseUrl}/source/rename?SourceID=${id}&newName=${encodeURIComponent(name)}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // Camera Source Controller routes (/api/cameraSource/*)
  async getCameraHardware(): Promise<CameraHardwareInfo[]> {
    const response = await fetch(`${this.baseUrl}/cameraSource/getNotRegistered`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getRegisteredCameraSources(): Promise<any[]> {
    const response = await fetch(`${this.baseUrl}/cameraSource/getRegistered`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createCameraSource(hardwareInfo: CameraHardwareInfo, name = 'default'): Promise<number> {
    const response = await fetch(`${this.baseUrl}/cameraSource/create?name=${encodeURIComponent(name)}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(hardwareInfo)
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createAllCameraSources(): Promise<void> {
    const response = await fetch(`${this.baseUrl}/cameraSource/createAll`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // Resolution/fps and exposure/gain control (CameraSourceController.cs) - backend-complete
  // since ROADMAP.md Phase 3b/B3, but never had a client method or any UI until now.
  async getCameraModes(id: number): Promise<CameraMode[]> {
    const response = await fetch(`${this.baseUrl}/cameraSource/${id}/modes`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getCameraCurrentMode(id: number): Promise<CameraMode> {
    const response = await fetch(`${this.baseUrl}/cameraSource/${id}/currentMode`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Returns whether the underlying ioctl/API call succeeded - NOT whether the device honoured
  // it exactly (both V4L2 and Media Foundation silently substitute the nearest mode). Re-GET
  // getCameraCurrentMode afterwards and check IsNative to see what was actually applied.
  async setCameraMode(id: number, mode: CameraMode): Promise<boolean> {
    const response = await fetch(`${this.baseUrl}/cameraSource/${id}/mode`, {
      method: 'PATCH',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(mode)
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // exposureAbsolute is in the backend's own native units (V4L2: 100us steps). Call
  // setCameraAutoExposure(id, false) first - most UVC hardware ignores a manual exposure value
  // while auto-exposure is still on.
  async setCameraExposure(id: number, exposureAbsolute: number): Promise<boolean> {
    const response = await fetch(`${this.baseUrl}/cameraSource/${id}/exposure?exposureAbsolute=${exposureAbsolute}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async setCameraAutoExposure(id: number, enabled: boolean): Promise<boolean> {
    const response = await fetch(`${this.baseUrl}/cameraSource/${id}/autoExposure?enabled=${enabled}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async setCameraGain(id: number, gain: number): Promise<boolean> {
    const response = await fetch(`${this.baseUrl}/cameraSource/${id}/gain?gain=${gain}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Video File Source Controller routes (/api/videoFileSource/*)
  async getAllVideoFileSources(): Promise<any[]> {
    const response = await fetch(`${this.baseUrl}/videoFileSource/getAll`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createVideoFileSource(files: FileList, fps: number = 30): Promise<number[]> {
    const formData = new FormData();
    Array.from(files).forEach(file => {
      formData.append('files', file);
    });
    
    const response = await fetch(`${this.baseUrl}/videoFileSource/create`, {
      method: 'POST',
      body: formData
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async changeVideoFileFPS(fps: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/videoFileSource/changeFPS?fps=${fps}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // Image File Source Controller routes (/api/imageFileSource/*)
  async getAllImageFileSources(): Promise<any[]> {
    const response = await fetch(`${this.baseUrl}/imageFileSource/get`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createImageFileSource(files: FileList): Promise<number[]> {
    const formData = new FormData();
    Array.from(files).forEach(file => {
      formData.append('files', file);
    });
    
    const response = await fetch(`${this.baseUrl}/imageFileSource/create`, {
      method: 'POST',
      body: formData
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Sink Controller routes
  async bindSinkToSource(sinkId: number, sourceId: number): Promise<void> {
      const response = await fetch(`${this.baseUrl}/sink/bind?SinkID=${sinkId}&SourceID=${sourceId}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async unbindSinkFromSource(sinkId: number, sourceId?: number): Promise<void> {
    const url = sourceId 
      ? `${this.baseUrl}/sink/unbind?SinkID=${sinkId}&SourceID=${sourceId}`
        : `${this.baseUrl}/sink/unbind?SinkID=${sinkId}`;
    const response = await fetch(url, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async deleteSink(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/delete?SinkID=${id}`, { method: 'DELETE' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // AprilTag Sink Controller routes
  async createApriltagSink(name: string, type: string): Promise<number> {
      const response = await fetch(`${this.baseUrl}/apriltagSink/create?name=${encodeURIComponent(name)}&type=apriltag`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Create an AprilTag sink with an explicit CPU/Vulkan backend and tag size - no calibration
  // data yet (see createApriltagSinkFromCalibrator for that). backend: 0 = CPU, 1 = Vulkan.
  async createApriltagSinkWithBackend(name: string, tagSize: number, backend: number, frameWidth = 0, frameHeight = 0): Promise<number> {
    const params = new URLSearchParams({ name, tagSize: String(tagSize), backend: String(backend), frameWidth: String(frameWidth), frameHeight: String(frameHeight) });
    const response = await fetch(`${this.baseUrl}/apriltagSink/createWithBackend?${params.toString()}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Which backend a sink actually ended up running - may differ from what was requested if
  // Vulkan was asked for and no usable device was found (falls back to CPU)
  async getApriltagBackendName(sinkId: number): Promise<string> {
    const response = await fetch(`${this.baseUrl}/apriltagSink/backend?sinkId=${sinkId}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Same as getApriltagBackendName, as the real 0/1 enum rather than a display string - lets the
  // Inspector's "Backend" dropdown pre-select the sink's actual current backend.
  async getApriltagBackendKind(sinkId: number): Promise<number> {
    const response = await fetch(`${this.baseUrl}/apriltagSink/backendKind?sinkId=${sinkId}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Switches an EXISTING sink between CPU/Vulkan in place, preserving its id/tag size/
  // calibration/bindings - the sink's own "Backend" control, not the Pipeline Profiles one
  // (createApriltagProfile's options.backend only helps if you set up a profile in advance).
  async setApriltagBackend(sinkId: number, backend: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/apriltagSink/backend?sinkId=${sinkId}&backend=${backend}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // Camera Calibration Sink Controller routes (default 6x9 checkerboard, 25mm squares - see
  // CreateWithBoard for a custom board, not exposed in the WebUI yet)
  async createCameraCalibrationSink(name: string): Promise<number> {
    const response = await fetch(`${this.baseUrl}/cameraCalibrationSink/create?name=${encodeURIComponent(name)}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createCameraCalibrationSinkWithBoard(name: string, boardType: number, rows: number, cols: number, squareSizeMeters: number): Promise<number> {
    const params = new URLSearchParams({ name, boardType: String(boardType), rows: String(rows), cols: String(cols), squareSizeMeters: String(squareSizeMeters) });
    const response = await fetch(`${this.baseUrl}/cameraCalibrationSink/createWithBoard?${params}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // ROADMAP.md Phase 8d: the camera calibration wizard's own capture/run/result loop - see
  // CameraCalibrationSinkController.cs. Mirrors the stereo equivalents below field-for-field.
  async saveCameraCalibrationDetection(sinkId: number): Promise<boolean> {
    const response = await fetch(`${this.baseUrl}/cameraCalibrationSink/${sinkId}/saveDetection`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getCameraCalibrationSnapshotCount(sinkId: number): Promise<number> {
    const response = await fetch(`${this.baseUrl}/cameraCalibrationSink/${sinkId}/snapshotCount`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async clearCameraCalibrationSnapshots(sinkId: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/cameraCalibrationSink/${sinkId}/snapshots`, { method: 'DELETE' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async runCameraCalibration(sinkId: number): Promise<CameraCalibrationResult> {
    const response = await fetch(`${this.baseUrl}/cameraCalibrationSink/${sinkId}/run`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getCameraCalibrationResult(sinkId: number): Promise<CameraCalibrationResult> {
    const response = await fetch(`${this.baseUrl}/cameraCalibrationSink/${sinkId}/result`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // the live coverage heatmap's own data source (ROADMAP.md Phase 8a/8d) - every saved
  // snapshot's/pair's detected corner points. `eye` only matters for the stereo endpoint.
  async getCameraCalibrationCoverage(sinkId: number): Promise<CalibrationCoverage> {
    const response = await fetch(`${this.baseUrl}/cameraCalibrationSink/${sinkId}/coverage`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getStereoCalibrationCoverage(sinkId: number, eye: 'left' | 'right'): Promise<CalibrationCoverage> {
    const response = await fetch(`${this.baseUrl}/stereoCalibrationSink/${sinkId}/coverage?eye=${eye}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Object Detection Sink Controller routes
  async createObjectDetectionSink(name: string, modelId: number): Promise<number> {
    const response = await fetch(`${this.baseUrl}/objectDetectionSink/create?name=${encodeURIComponent(name)}&modelId=${modelId}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Model Controller routes (uploaded YOLOv8/YOLOv11 ONNX models, consumed by object detection sinks)
  async getAllModels(): Promise<Model[]> {
    const response = await fetch(`${this.baseUrl}/model/getAll`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async uploadModel(params: {
    name: string;
    variant: number; // 0 = YOLOv8, 1 = YOLOv11
    inputSize?: number;
    confThreshold?: number;
    nmsThreshold?: number;
    modelFile: File;
    labelsFile?: File;
  }): Promise<number> {
    const formData = new FormData();
    formData.append('name', params.name);
    formData.append('variant', String(params.variant));
    formData.append('inputSize', String(params.inputSize ?? 640));
    formData.append('confThreshold', String(params.confThreshold ?? 0.25));
    formData.append('nmsThreshold', String(params.nmsThreshold ?? 0.45));
    formData.append('model', params.modelFile);
    if (params.labelsFile) formData.append('labels', params.labelsFile);

    const response = await fetch(`${this.baseUrl}/model/upload`, { method: 'POST', body: formData });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async deleteModel(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/model/delete?id=${id}`, { method: 'DELETE' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // NetworkTables Sink Controller routes
  async createNetworkTablesSinkForTeam(name: string, teamNumber: number, rootTable = 'lumenvision', clientIdentity = 'lumenvision'): Promise<number> {
    const params = new URLSearchParams({ name, teamNumber: String(teamNumber), rootTable, clientIdentity });
    const response = await fetch(`${this.baseUrl}/networkTablesSink/createForTeam?${params.toString()}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createNetworkTablesSinkForServer(name: string, serverAddress: string, port = 0, rootTable = 'lumenvision', clientIdentity = 'lumenvision'): Promise<number> {
    const params = new URLSearchParams({ name, serverAddress, port: String(port), rootTable, clientIdentity });
    const response = await fetch(`${this.baseUrl}/networkTablesSink/createForServer?${params.toString()}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // ROADMAP.md Phase 8e: this used to double-JSON.parse() the response, matching what
  // NetworkTablesSinkController.GetStatus returned before the Phase 8a DTO cleanup - it's
  // returned a real typed NetworkTablesStatusDto since then (a plain object, not a JSON-encoded
  // string), so the extra parse was left silently broken (throwing on an already-parsed object)
  // until Phase 8e's match view became the first thing to actually call this.
  async getNetworkTablesStatus(sinkId: number): Promise<NetworkTablesStatus> {
    const response = await fetch(`${this.baseUrl}/networkTablesSink/status?sinkId=${sinkId}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // WebRTC Sink Controller routes
  async createWebRTCSink(name: string, bitrateKbps = 4000, fps = 30, encoderName = 'libx264'): Promise<number> {
    const params = new URLSearchParams({ name, bitrateKbps: String(bitrateKbps), fps: String(fps), encoderName });
    const response = await fetch(`${this.baseUrl}/webrtcSink/create?${params.toString()}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Blocks briefly server-side for ICE gathering (non-trickle on LumenVision's side). Returned as a
  // raw text/plain body, not JSON - SDP is full of literal \r\n line endings that EmbedIO's
  // default string auto-serialization does not escape, which makes response.json() fail with
  // "Bad control character in string literal" (confirmed the hard way).
  async getWebRTCOffer(sinkId: number): Promise<string> {
    const response = await fetch(`${this.baseUrl}/webrtcSink/offer?sinkId=${sinkId}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.text();
  }

  async sendWebRTCAnswer(sinkId: number, sdp: string): Promise<void> {
    const response = await fetch(`${this.baseUrl}/webrtcSink/answer?sinkId=${sinkId}`, {
      method: 'POST',
      headers: { 'Content-Type': 'text/plain' },
      body: sdp
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async sendWebRTCIceCandidate(sinkId: number, candidate: string, mid: string): Promise<void> {
    const params = new URLSearchParams({ sinkId: String(sinkId), candidate, mid: mid ?? '' });
    const response = await fetch(`${this.baseUrl}/webrtcSink/candidate?${params.toString()}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // same fix as getNetworkTablesStatus above - WebRTCSinkController.GetStatus also returns a
  // real WebRtcStatusDto since Phase 8a, not a double-JSON-encoded string. Nothing currently
  // calls this (the Inspector reads WebRTC running state off the /ws/state snapshot instead),
  // but it's a real bug regardless if left as-is for whenever something does.
  async getWebRTCSinkStatus(sinkId: number): Promise<{ Connected: boolean; IceState: number; GatheringComplete: boolean }> {
    const response = await fetch(`${this.baseUrl}/webrtcSink/status?sinkId=${sinkId}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Stereo Calibration Sink Controller routes (phase 10 - see STEREO_IMPLEMENTATION_PLAN.md)
  async createStereoCalibrationSink(name: string): Promise<number> {
    const response = await fetch(`${this.baseUrl}/stereoCalibrationSink/create?name=${encodeURIComponent(name)}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // ChArUco is not supported for stereo yet (StereoCalibrator.h) - boardType is always the
  // checkerboard value (0) from this WebUI.
  async createStereoCalibrationSinkWithBoard(name: string, rows: number, cols: number, squareSizeMeters: number): Promise<number> {
    const params = new URLSearchParams({ name, boardType: '0', rows: String(rows), cols: String(cols), squareSizeMeters: String(squareSizeMeters) });
    const response = await fetch(`${this.baseUrl}/stereoCalibrationSink/createWithBoard?${params.toString()}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // binds the explicit left/right camera sources for a stereo sink (StereoCalibrationSink or
  // StereoDepthSink) - the ordinary bindSinkToSource doesn't apply here, since getting
  // left/right backwards silently flips the sign of every disparity.
  async bindStereoSources(sinkId: number, leftSourceId: number, rightSourceId: number): Promise<void> {
    const params = new URLSearchParams({ leftSourceId: String(leftSourceId), rightSourceId: String(rightSourceId) });
    const response = await fetch(`${this.baseUrl}/stereoCalibrationSink/${sinkId}/bind?${params.toString()}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async bindStereoDepthSources(sinkId: number, leftSourceId: number, rightSourceId: number): Promise<void> {
    const params = new URLSearchParams({ leftSourceId: String(leftSourceId), rightSourceId: String(rightSourceId) });
    const response = await fetch(`${this.baseUrl}/stereoDepthSink/${sinkId}/bind?${params.toString()}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async saveStereoCalibrationDetection(sinkId: number): Promise<boolean> {
    const response = await fetch(`${this.baseUrl}/stereoCalibrationSink/${sinkId}/saveDetection`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getStereoCalibrationPairCount(sinkId: number): Promise<number> {
    const response = await fetch(`${this.baseUrl}/stereoCalibrationSink/${sinkId}/pairCount`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async clearStereoCalibrationPairs(sinkId: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/stereoCalibrationSink/${sinkId}/pairs`, { method: 'DELETE' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // runs cv::stereoCalibrate + cv::stereoRectify over every saved pair. Check the returned
  // epipolarRms - gate real use at < 0.5px (STEREO_IMPLEMENTATION_PLAN.md ss10.2); stereoRms
  // alone does not predict codec-stereo density/validity the way epipolarRms does.
  async runStereoCalibration(sinkId: number): Promise<StereoCalibrationResult> {
    const response = await fetch(`${this.baseUrl}/stereoCalibrationSink/${sinkId}/run`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getStereoCalibrationResult(sinkId: number): Promise<StereoCalibrationResult> {
    const response = await fetch(`${this.baseUrl}/stereoCalibrationSink/${sinkId}/result`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Stereo Depth Sink Controller routes
  async createStereoDepthSink(params: {
    name: string; backend: number; minDepthMeters: number; maxDepthMeters: number;
    maxSkewUs: number; frameOutput: number; calibration: StereoCalibrationResult;
  }): Promise<number> {
    const query = new URLSearchParams({
      name: params.name, backend: String(params.backend),
      minDepthMeters: String(params.minDepthMeters), maxDepthMeters: String(params.maxDepthMeters),
      maxSkewUs: String(params.maxSkewUs), frameOutput: String(params.frameOutput),
    });
    const response = await fetch(`${this.baseUrl}/stereoDepthSink/create?${query.toString()}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(params.calibration),
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getStereoDepthBackendName(sinkId: number): Promise<string> {
    const response = await fetch(`${this.baseUrl}/stereoDepthSink/${sinkId}/backendName`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getStereoDepthStats(sinkId: number): Promise<StereoDepthStats> {
    const response = await fetch(`${this.baseUrl}/stereoDepthSink/${sinkId}/stats`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Depth Fusion Sink Controller routes - fuses a detector's boxes with a StereoDepthSink's
  // depth grid (STEREO_IMPLEMENTATION_PLAN.md ss10.4). Bind the detector with the ordinary
  // bindSinkToSource (it must itself be bound to the StereoDepthSink's rectified-left frame
  // output, not a raw camera); attach the depth source separately.
  async createDepthFusionSink(name: string): Promise<number> {
    const response = await fetch(`${this.baseUrl}/depthFusionSink/create?name=${encodeURIComponent(name)}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async attachDepthFusionSource(fusionSinkId: number, stereoDepthSinkId: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/depthFusionSink/${fusionSinkId}/attachDepthSource?stereoDepthSinkId=${stereoDepthSinkId}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // Device Controller routes
  async getDeviceCPUUsage(): Promise<number> {
    const response = await fetch(`${this.baseUrl}/device/cpuUsage`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getDeviceRAMUsage(): Promise<number> {
    const response = await fetch(`${this.baseUrl}/device/ramUsage`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getDeviceDiskUsage(): Promise<number> {
    const response = await fetch(`${this.baseUrl}/device/diskUsage`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // UDP Controller routes
  async startUDPTransmission(): Promise<void> {
    const response = await fetch(`${this.baseUrl}/udp/start`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async stopUDPTransmission(): Promise<void> {
    const response = await fetch(`${this.baseUrl}/udp/stop`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // Enhanced utility methods for better data aggregation
  async getAllSources(): Promise<any[]> {
    try {
      // Get sources from all specific endpoints
      const [mainSources, cameras, videos, images] = await Promise.allSettled([
        this.getSources(),
        this.getRegisteredCameraSources(),
        this.getAllVideoFileSources(),
        this.getAllImageFileSources()
      ]);
      
      let allSources: any[] = [];
      
      // Add main sources if available
      if (mainSources.status === 'fulfilled') {
        allSources = [...allSources, ...mainSources.value];
      }
      
      // Add camera sources
      if (cameras.status === 'fulfilled') {
        allSources = [...allSources, ...cameras.value];
      }
      
      // Add video file sources
      if (videos.status === 'fulfilled') {
        allSources = [...allSources, ...videos.value];
      }
      
      // Add image file sources
      if (images.status === 'fulfilled') {
        allSources = [...allSources, ...images.value];
      }
      
      // Remove duplicates based on ID
      const uniqueSources = allSources.filter((source, index, self) => 
        index === self.findIndex(s => (s.Id || s.id) === (source.Id || source.id))
      );
      
      return uniqueSources;
    } catch (error) {
      console.error('Failed to load sources:', error);
      return [];
    }
  }

  async getAllSinks(): Promise<any[]> {
      const response = await fetch(`${this.baseUrl}/sink/getAll`, { method: 'GET' });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      return response.json();
  }

  // Improved file upload methods with better handling
  async uploadVideoFiles(files: FileList, fps: number = 30): Promise<{
    success: boolean;
    sourceIds: number[];
    message: string;
  }> {
    try {
      const sourceIds = await this.createVideoFileSource(files, fps);
      return {
        success: true,
        sourceIds,
        message: `Successfully uploaded ${files.length} video file(s) with IDs: ${sourceIds.join(', ')}`
      };
    } catch (error) {
      return {
        success: false,
        sourceIds: [],
        message: `Failed to upload video files: ${error}`
      };
    }
  }

  async uploadImageFiles(files: FileList): Promise<{
    success: boolean;
    sourceIds: number[];
    message: string;
  }> {
    try {
      const sourceIds = await this.createImageFileSource(files);
      return {
        success: true,
        sourceIds,
        message: `Successfully uploaded ${files.length} image file(s) with IDs: ${sourceIds.join(', ')}`
      };
    } catch (error) {
      return {
        success: false,
        sourceIds: [],
        message: `Failed to upload image files: ${error}`
      };
    }
  }

  // Enhanced file validation
  validateVideoFile(file: File): { valid: boolean; message: string } {
    const validVideoTypes = ['video/mp4', 'video/avi', 'video/mov', 'video/wmv', 'video/mkv', 'video/webm'];
    const maxSize = 500 * 1024 * 1024; // 500MB
    
    if (!validVideoTypes.includes(file.type)) {
      return {
        valid: false,
        message: `Invalid video format. Supported formats: ${validVideoTypes.join(', ')}`
      };
    }
    
    if (file.size > maxSize) {
      return {
        valid: false,
        message: `File too large. Maximum size: ${maxSize / 1024 / 1024}MB`
      };
    }
    
    return { valid: true, message: 'Valid video file' };
  }

  validateImageFile(file: File): { valid: boolean; message: string } {
    const validImageTypes = ['image/jpeg', 'image/jpg', 'image/png', 'image/bmp', 'image/gif', 'image/webp'];
    const maxSize = 50 * 1024 * 1024; // 50MB
    
    if (!validImageTypes.includes(file.type)) {
      return {
        valid: false,
        message: `Invalid image format. Supported formats: ${validImageTypes.join(', ')}`
      };
    }
    
    if (file.size > maxSize) {
      return {
        valid: false,
        message: `File too large. Maximum size: ${maxSize / 1024 / 1024}MB`
      };
    }
    
    return { valid: true, message: 'Valid image file' };
  }

  // Legacy compatibility methods
  async getSource(id: number): Promise<any> {
    const sources = await this.getAllSources();
    return sources.find(s => (s.Id || s.id) === id) || null;
  }

  async getSinks(): Promise<any[]> {
    return this.getAllSinks();
  }

  async getSink(id: number): Promise<any> {
    const sinks = await this.getAllSinks();
    return sinks.find(s => (s.Id || s.id) === id) || null;
  }

  async addSink(name: string, type: string): Promise<number> {
    return this.createApriltagSink(name, type);
  }

  async changeSourceName(id: number, name: string): Promise<void> {
    return this.renameSource(id, name);
  }

  async renameSink(id: number, name: string): Promise<void> {
        const response = await fetch(`${this.baseUrl}/sink/rename?NewName=${encodeURIComponent(name)}&SinkID=${encodeURIComponent(id)}`, { method: 'PATCH' });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // the sink's latest result (Manager::GetSinkResult, already a JSON document) - its shape
  // varies per sink type (AprilTag detections vs. object-detection boxes vs. ...), so unlike
  // NetworkTablesSink/WebRTCSink's own fixed-shape status there's no one DTO to type this as;
  // SinkController writes the response body directly rather than letting it go through
  // EmbedIO's default string serializer, which does not escape embedded quotes correctly (see
  // that controller's own comment - confirmed the hard way, this endpoint went unexercised by
  // the webui until Phase 8c actually started calling it).
  async getSinkResult(sinkId: number): Promise<unknown> {
    const response = await fetch(`${this.baseUrl}/sink/getResult?SinkID=${sinkId}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getSinkStatus(sinkId: number): Promise<boolean> {
    const response = await fetch(`${this.baseUrl}/sink/getStatus?SinkID=${sinkId}`, { method: 'GET' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async toggleSink(sinkId: number, enabled: boolean): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/toggle?SinkID=${sinkId}&Enabled=${enabled}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async enableSink(id: number): Promise<void> {
      const response = await fetch(`${this.baseUrl}/sink/toggle?SinkID=${encodeURIComponent(id)}&Enabled=${true}`, { method: 'PATCH' });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async disableSink(id: number): Promise<void> {
      const response = await fetch(`${this.baseUrl}/sink/toggle?SinkID=${encodeURIComponent(id)}&Enabled=${false}`, { method: 'PATCH' });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // Capabilities Controller routes (/api/capabilities/*) - ROADMAP.md Phase 8a/8c
  async getNodeTypeCapabilities(): Promise<NodeTypesResponse> {
    const response = await fetch(`${this.baseUrl}/capabilities/nodeTypes`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getEnabledFeatures(): Promise<string[]> {
    const response = await fetch(`${this.baseUrl}/capabilities/features`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  // Pipeline Profile Controller routes (/api/source/profiles*) - ROADMAP.md Phase 7/8c. A
  // profile belongs to a camera SOURCE (not a sink) - activating one tears down and rebuilds
  // whatever detection sink is currently bound to that source. See PipelineProfileController.cs.
  async getProfiles(sourceId: number): Promise<PipelineProfile[]> {
    const response = await fetch(`${this.baseUrl}/source/profiles?sourceId=${sourceId}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createApriltagProfile(sourceId: number, name: string, tagSize: number, options?: {
    calibratorSinkId?: number; backend?: number; frameWidth?: number; frameHeight?: number; driverMode?: boolean;
  }): Promise<number> {
    const params = new URLSearchParams({ sourceId: String(sourceId), name, tagSize: String(tagSize) });
    if (options?.calibratorSinkId != null) params.set('calibratorSinkId', String(options.calibratorSinkId));
    if (options?.backend != null) params.set('backend', String(options.backend));
    if (options?.frameWidth != null) params.set('frameWidth', String(options.frameWidth));
    if (options?.frameHeight != null) params.set('frameHeight', String(options.frameHeight));
    if (options?.driverMode != null) params.set('driverMode', String(options.driverMode));
    const response = await fetch(`${this.baseUrl}/source/profiles/apriltag?${params}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createObjectDetectionProfile(sourceId: number, name: string, modelId: number): Promise<number> {
    const params = new URLSearchParams({ sourceId: String(sourceId), name, modelId: String(modelId) });
    const response = await fetch(`${this.baseUrl}/source/profiles/objectDetection?${params}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async activateProfile(sourceId: number, index: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/source/profiles/activate?sourceId=${sourceId}&index=${index}`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async deleteProfile(sourceId: number, index: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/source/profiles?sourceId=${sourceId}&index=${index}`, { method: 'DELETE' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

}