import type { CameraHardwareInfo } from '../types';

export class ApiService {
  private baseUrl = 'http://localhost:8175/api';

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

  async createCameraSource(hardwareInfo: CameraHardwareInfo): Promise<number> {
    const response = await fetch(`${this.baseUrl}/cameraSource/create`, {
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

  async enableSink(id: number): Promise<void> {
      const response = await fetch(`${this.baseUrl}/sink/toggle?SinkID=${encodeURIComponent(id)}&Enabled=${true}`, { method: 'PATCH' });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async disableSink(id: number): Promise<void> {
      const response = await fetch(`${this.baseUrl}/sink/toggle?SinkID=${encodeURIComponent(id)}&Enabled=${false}`, { method: 'PATCH' });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // WebRTC methods - not implemented in current API
  async startWebRTCStream(sinkId: number): Promise<any> {
    throw new Error('WebRTC streaming not implemented in current API');
  }

  async stopWebRTCStream(sinkId: number): Promise<any> {
    throw new Error('WebRTC streaming not implemented in current API');
  }

  async getWebRTCStatus(sinkId: number): Promise<any> {
    throw new Error('WebRTC status not implemented in current API');
  }

  async enablePreview(sinkId: number): Promise<any> {
    throw new Error('Preview functionality not implemented in current API');
  }

  async disablePreview(sinkId: number): Promise<any> {
    throw new Error('Preview functionality not implemented in current API');
  }
}