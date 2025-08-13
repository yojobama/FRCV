import type { CameraHardwareInfo } from '../types';

export class ApiService {
  private baseUrl = 'http://localhost:8175/api';

  async getSources(): Promise<number[]> {
    const response = await fetch(`${this.baseUrl}/source/ids`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getSinks(): Promise<number[]> {
    const response = await fetch(`${this.baseUrl}/sink/ids`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getSink(id: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/getSinkById?sinkId=${id}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getSource(id: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/source?sourceId=${id}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const text = await response.text();
    return text ? JSON.parse(text) : null;
  }

  async getCameraHardware(): Promise<CameraHardwareInfo[]> {
    const response = await fetch(`${this.baseUrl}/source/camera/hardwareInfos`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async enableSink(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/enable?sinkId=${id}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async disableSink(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/disable?sinkId=${id}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async addSink(name: string, type: string): Promise<number> {
    const response = await fetch(`${this.baseUrl}/sink/add?name=${encodeURIComponent(name)}&type=${encodeURIComponent(type)}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async deleteSink(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/delete?sinkId=${id}`, { method: 'DELETE' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async changeSinkName(id: number, name: string): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/changeName?sinkId=${id}&name=${encodeURIComponent(name)}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async changeSourceName(id: number, name: string): Promise<void> {
    const response = await fetch(`${this.baseUrl}/source/changeName?sourceId=${id}&name=${encodeURIComponent(name)}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  // Source creation methods
  async createCameraSource(hardwareInfo: CameraHardwareInfo): Promise<number> {
    const response = await fetch(`${this.baseUrl}/source/createCameraSource`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(hardwareInfo)
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createVideoFileSource(file: File, fps: number = 30): Promise<number> {
    const formData = new FormData();
    formData.append('file', file);
    
    const response = await fetch(`${this.baseUrl}/source/createVideoFileSource?fps=${fps}`, {
      method: 'POST',
      body: formData
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async createImageFileSource(file: File): Promise<number> {
    const formData = new FormData();
    formData.append('file', file);
    
    const response = await fetch(`${this.baseUrl}/source/createImageFileSource`, {
      method: 'POST',
      body: formData
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async deleteSource(id: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/source/delete?sourceId=${id}`, { method: 'DELETE' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async bindSinkToSource(sinkId: number, sourceId: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/bind?sinkId=${sinkId}&sourceId=${sourceId}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async unbindSinkFromSource(sinkId: number, sourceId: number): Promise<void> {
    const response = await fetch(`${this.baseUrl}/sink/unbind?sinkId=${sinkId}&sourceId=${sourceId}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
  }

  async startWebRTCStream(sinkId: number): Promise<any> {
    const connectionId = `conn_${Math.random().toString(36).substr(2, 9)}`;
    const response = await fetch(`${this.baseUrl}/sink/webrtc/start?sinkId=${sinkId}&connectionId=${connectionId}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async stopWebRTCStream(sinkId: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/webrtc/stop?sinkId=${sinkId}`, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async getWebRTCStatus(sinkId: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/webrtc/status?sinkId=${sinkId}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async enablePreview(sinkId: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/preview/enable?sinkId=${sinkId}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }

  async disablePreview(sinkId: number): Promise<any> {
    const response = await fetch(`${this.baseUrl}/sink/preview/disable?sinkId=${sinkId}`, { method: 'PUT' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }
}