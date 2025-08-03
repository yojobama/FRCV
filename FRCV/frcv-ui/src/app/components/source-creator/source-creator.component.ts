import { Component, OnInit, Output, EventEmitter } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';

interface CameraHardwareInfo {
  name: string;
  path: string;
}

@Component({
  selector: 'app-source-creator',
  template: `
    <div class="source-creator">
      <h3>Create New Source</h3>
      
      <form [formGroup]="sourceForm" (ngSubmit)="onCreateSource()">
        <div class="form-group">
          <label for="sourceType">Source Type:</label>
          <select id="sourceType" formControlName="type" class="form-control">
            <option value="camera">Camera</option>
            <option value="video">Video File</option>
            <option value="image">Image File</option>
          </select>
        </div>

        <!-- Camera Selection -->
        <div *ngIf="sourceForm.get('type')?.value === 'camera'" class="form-group">
          <label for="cameraSelect">Select Camera:</label>
          <select id="cameraSelect" formControlName="selectedCamera" class="form-control">
            <option value="">Select a camera...</option>
            <option *ngFor="let camera of availableCameras" [value]="camera | json">
              {{camera.name}} ({{camera.path}})
            </option>
          </select>
          <button type="button" (click)="loadCameras()" class="btn btn-secondary">
            Refresh Cameras
          </button>
        </div>

        <!-- File Selection -->
        <div *ngIf="sourceForm.get('type')?.value === 'video' || sourceForm.get('type')?.value === 'image'" 
             class="form-group">
          <label for="fileInput">Select File:</label>
          <input 
            id="fileInput"
            type="file" 
            (change)="onFileSelected($event)"
            [accept]="sourceForm.get('type')?.value === 'video' ? 'video/*' : 'image/*'"
            class="form-control"
          />
        </div>

        <!-- FPS for Video -->
        <div *ngIf="sourceForm.get('type')?.value === 'video'" class="form-group">
          <label for="fps">FPS:</label>
          <input 
            id="fps"
            type="number" 
            formControlName="fps" 
            min="1" 
            max="120" 
            class="form-control"
          />
        </div>

        <button 
          type="submit" 
          class="btn btn-primary"
          [disabled]="sourceForm.invalid || isCreating"
        >
          {{ isCreating ? 'Creating...' : 'Create Source' }}
        </button>
      </form>

      <div *ngIf="message" class="alert" [ngClass]="messageType">
        {{ message }}
      </div>
    </div>
  `,
  styles: [`
    .source-creator {
      background: #f8f9fa;
      padding: 2rem;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      margin-bottom: 2rem;
    }

    h3 {
      text-align: center;
      margin-bottom: 1.5rem;
      color: #333;
    }

    .form-group {
      margin-bottom: 1.5rem;
    }

    label {
      display: block;
      margin-bottom: 0.5rem;
      font-weight: bold;
      color: #555;
    }

    .form-control {
      width: 100%;
      padding: 0.75rem;
      border: 1px solid #ddd;
      border-radius: 4px;
      font-size: 1rem;
    }

    .form-control:focus {
      outline: none;
      border-color: #007bff;
      box-shadow: 0 0 0 2px rgba(0,123,255,0.25);
    }

    .btn {
      padding: 0.75rem 1.5rem;
      border: none;
      border-radius: 4px;
      font-size: 1rem;
      cursor: pointer;
      margin-right: 0.5rem;
      margin-top: 0.5rem;
    }

    .btn-primary {
      background-color: #007bff;
      color: white;
      width: 100%;
    }

    .btn-primary:hover:not(:disabled) {
      background-color: #0056b3;
    }

    .btn-secondary {
      background-color: #6c757d;
      color: white;
    }

    .btn-secondary:hover {
      background-color: #545b62;
    }

    .btn:disabled {
      background-color: #6c757d;
      cursor: not-allowed;
    }

    .alert {
      margin-top: 1rem;
      padding: 0.75rem;
      border-radius: 4px;
    }

    .alert-success {
      background-color: #d4edda;
      color: #155724;
      border: 1px solid #c3e6cb;
    }

    .alert-danger {
      background-color: #f8d7da;
      color: #721c24;
      border: 1px solid #f5c6cb;
    }
  `]
})
export class SourceCreatorComponent implements OnInit {
  @Output() sourceCreated = new EventEmitter<number>();

  sourceForm: FormGroup;
  availableCameras: CameraHardwareInfo[] = [];
  selectedFile: File | null = null;
  isCreating = false;
  message = '';
  messageType = '';

  constructor(private formBuilder: FormBuilder) {
    this.sourceForm = this.formBuilder.group({
      type: ['camera', Validators.required],
      selectedCamera: [''],
      fps: [30, [Validators.min(1), Validators.max(120)]]
    });
  }

  ngOnInit(): void {
    this.loadCameras();
  }

  async loadCameras(): Promise<void> {
    try {
      const response = await fetch('http://localhost:8080/api/source/camera/hardwareInfos');
      this.availableCameras = await response.json();
      
      if (this.availableCameras.length > 0) {
        this.sourceForm.patchValue({
          selectedCamera: JSON.stringify(this.availableCameras[0])
        });
      }
    } catch (error) {
      console.error('Error loading cameras:', error);
      this.showMessage('Error loading cameras', 'alert-danger');
    }
  }

  onFileSelected(event: any): void {
    const file = event.target.files[0];
    if (file) {
      this.selectedFile = file;
    }
  }

  async onCreateSource(): Promise<void> {
    if (this.sourceForm.invalid) {
      return;
    }

    this.isCreating = true;
    this.message = '';

    try {
      const sourceType = this.sourceForm.get('type')?.value;
      let sourceId: number;

      if (sourceType === 'camera') {
        const selectedCamera = this.sourceForm.get('selectedCamera')?.value;
        if (!selectedCamera) {
          this.showMessage('Please select a camera', 'alert-danger');
          return;
        }
        
        const cameraInfo = JSON.parse(selectedCamera);
        sourceId = await this.createCameraSource(cameraInfo);
      } else if (sourceType === 'video') {
        if (!this.selectedFile) {
          this.showMessage('Please select a video file', 'alert-danger');
          return;
        }
        
        const fps = this.sourceForm.get('fps')?.value || 30;
        sourceId = await this.createVideoFileSource(this.selectedFile, fps);
      } else if (sourceType === 'image') {
        if (!this.selectedFile) {
          this.showMessage('Please select an image file', 'alert-danger');
          return;
        }
        
        sourceId = await this.createImageFileSource(this.selectedFile);
      } else {
        throw new Error('Invalid source type');
      }

      this.showMessage(`Source created successfully with ID: ${sourceId}`, 'alert-success');
      this.sourceCreated.emit(sourceId);
      this.resetForm();
    } catch (error) {
      console.error('Error creating source:', error);
      this.showMessage('Failed to create source. Please try again.', 'alert-danger');
    } finally {
      this.isCreating = false;
    }
  }

  private async createCameraSource(hardwareInfo: CameraHardwareInfo): Promise<number> {
    const response = await fetch('http://localhost:8080/api/source/createCameraSource', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(hardwareInfo)
    });
    
    if (!response.ok) {
      throw new Error('Failed to create camera source');
    }
    
    return response.json();
  }

  private async createVideoFileSource(file: File, fps: number): Promise<number> {
    const formData = new FormData();
    formData.append('file', file);
    
    const response = await fetch(`http://localhost:8080/api/source/createVideoFileSource?fps=${fps}`, {
      method: 'POST',
      body: formData
    });
    
    if (!response.ok) {
      throw new Error('Failed to create video source');
    }
    
    return response.json();
  }

  private async createImageFileSource(file: File): Promise<number> {
    const formData = new FormData();
    formData.append('file', file);
    
    const response = await fetch('http://localhost:8080/api/source/createImageFileSource', {
      method: 'POST',
      body: formData
    });
    
    if (!response.ok) {
      throw new Error('Failed to create image source');
    }
    
    return response.json();
  }

  private showMessage(message: string, type: string): void {
    this.message = message;
    this.messageType = type;
    
    setTimeout(() => {
      this.message = '';
      this.messageType = '';
    }, 5000);
  }

  private resetForm(): void {
    this.sourceForm.reset({
      type: 'camera',
      fps: 30
    });
    this.selectedFile = null;
    
    if (this.availableCameras.length > 0) {
      this.sourceForm.patchValue({
        selectedCamera: JSON.stringify(this.availableCameras[0])
      });
    }
  }
}