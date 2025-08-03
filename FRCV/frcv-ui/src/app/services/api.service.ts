import { Injectable } from '@angular/core';
import { HttpClient, HttpParams } from '@angular/common/http';
import { Observable } from 'rxjs';

export interface CameraHardwareInfo {
  name: string;
  path: string;
}

export interface Sink {
  id: number;
  name: string;
  type: string;
  source?: any;
}

export interface Source {
  id: number;
  name: string;
  type: string;
}

@Injectable({
  providedIn: 'root'
})
export class ApiService {
  private baseUrl = 'http://localhost:8080/api'; // Adjust based on your server configuration

  constructor(private http: HttpClient) { }

  // Sink APIs
  getSinks(): Observable<number[]> {
    return this.http.get<number[]>(`${this.baseUrl}/sink/ids`);
  }

  getSinkById(sinkId: number): Observable<Sink> {
    const params = new HttpParams().set('sinkId', sinkId.toString());
    return this.http.get<Sink>(`${this.baseUrl}/sink/getSinkById`, { params });
  }

  createSink(name: string, type: string): Observable<number> {
    const params = new HttpParams()
      .set('name', name)
      .set('type', type);
    return this.http.post<number>(`${this.baseUrl}/sink/add`, null, { params });
  }

  deleteSink(sinkId: number): Observable<void> {
    const params = new HttpParams().set('sinkId', sinkId.toString());
    return this.http.delete<void>(`${this.baseUrl}/sink/delete`, { params });
  }

  changeSinkName(sinkId: number, name: string): Observable<void> {
    const params = new HttpParams()
      .set('sinkId', sinkId.toString())
      .set('name', name);
    return this.http.put<void>(`${this.baseUrl}/sink/changeName`, null, { params });
  }

  enableSink(sinkId: number): Observable<void> {
    const params = new HttpParams().set('sinkId', sinkId.toString());
    return this.http.put<void>(`${this.baseUrl}/sink/enable`, null, { params });
  }

  disableSink(sinkId: number): Observable<void> {
    const params = new HttpParams().set('sinkId', sinkId.toString());
    return this.http.put<void>(`${this.baseUrl}/sink/disable`, null, { params });
  }

  bindSinkToSource(sourceId: number, sinkId: number): Observable<void> {
    const params = new HttpParams()
      .set('sourceId', sourceId.toString())
      .set('sinkId', sinkId.toString());
    return this.http.put<void>(`${this.baseUrl}/sink/bind`, null, { params });
  }

  unbindSinkFromSource(sinkId: number, sourceId: number): Observable<void> {
    const params = new HttpParams()
      .set('sinkId', sinkId.toString())
      .set('sourceId', sourceId.toString());
    return this.http.put<void>(`${this.baseUrl}/sink/unbind`, null, { params });
  }

  // Source APIs
  getSources(): Observable<number[]> {
    return this.http.get<number[]>(`${this.baseUrl}/source/ids`);
  }

  getSourceById(sourceId: number): Observable<string> {
    const params = new HttpParams().set('sourceId', sourceId.toString());
    return this.http.get<string>(`${this.baseUrl}/source`, { params });
  }

  getCameraHardwareInfos(): Observable<CameraHardwareInfo[]> {
    return this.http.get<CameraHardwareInfo[]>(`${this.baseUrl}/source/camera/hardwareInfos`);
  }

  createCameraSource(hardwareInfo: CameraHardwareInfo): Observable<number> {
    return this.http.post<number>(`${this.baseUrl}/source/createCameraSource`, hardwareInfo);
  }

  createVideoFileSource(file: File, fps: number): Observable<number> {
    const formData = new FormData();
    formData.append('file', file);
    
    const params = new HttpParams().set('fps', fps.toString());
    return this.http.post<number>(`${this.baseUrl}/source/createVideoFileSource`, formData, { params });
  }

  createImageFileSource(file: File): Observable<number> {
    const formData = new FormData();
    formData.append('file', file);
    
    return this.http.post<number>(`${this.baseUrl}/source/createImageFileSource`, formData);
  }

  changeSourceName(sourceId: number, name: string): Observable<void> {
    const params = new HttpParams()
      .set('sourceId', sourceId.toString())
      .set('name', name);
    return this.http.put<void>(`${this.baseUrl}/source/changeName`, null, { params });
  }

  deleteSource(sourceId: number): Observable<void> {
    const params = new HttpParams().set('sourceId', sourceId.toString());
    return this.http.delete<void>(`${this.baseUrl}/source/delete`, { params });
  }
}