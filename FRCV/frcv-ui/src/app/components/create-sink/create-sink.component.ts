import { Component, OnInit } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { ApiService, CameraHardwareInfo } from '../../services/api.service';

@Component({
  selector: 'app-create-sink',
  template: `
    <div class="create-sink-container">
      <h2>Create New Sink</h2>
      
      <form [formGroup]="sinkForm" (ngSubmit)="onCreateSink()">
        <div class="form-group">
          <label for="sinkName">Sink Name:</label>
          <input 
            id="sinkName"
            type="text" 
            formControlName="name" 
            class="form-control"
            placeholder="Enter sink name"
          />
          <div *ngIf="sinkForm.get('name')?.invalid && sinkForm.get('name')?.touched" 
               class="error-message">
            Sink name is required
          </div>
        </div>

        <div class="form-group">
          <label for="sinkType">Sink Type:</label>
          <select id="sinkType" formControlName="type" class="form-control">
            <option value="ApriltagSink">Apriltag Sink</option>
            <option value="ObjectDetectionSink">Object Detection Sink</option>
          </select>
        </div>

        <button 
          type="submit" 
          class="btn btn-primary"
          [disabled]="sinkForm.invalid || isCreating"
        >
          {{ isCreating ? 'Creating...' : 'Create Sink' }}
        </button>
      </form>

      <div *ngIf="message" class="message" [ngClass]="messageType">
        {{ message }}
      </div>
    </div>
  `,
  styles: [`
    .create-sink-container {
      max-width: 600px;
      margin: 0 auto;
      padding: 2rem;
      background: #f8f9fa;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }

    h2 {
      text-align: center;
      margin-bottom: 2rem;
      colour: #333;
    }

    .form-group {
      margin-bottom: 1.5rem;
    }

    label {
      display: block;
      margin-bottom: 0.5rem;
      font-weight: bold;
      colour: #555;
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
      border-colour: #007bff;
      box-shadow: 0 0 0 2px rgba(0,123,255,0.25);
    }

    .btn {
      padding: 0.75rem 1.5rem;
      border: none;
      border-radius: 4px;
      font-size: 1rem;
      cursor: pointer;
      width: 100%;
      transition: background-colour 0.3s;
    }

    .btn-primary {
      background-colour: #007bff;
      colour: white;
    }

    .btn-primary:hover:not(:disabled) {
      background-colour: #0056b3;
    }

    .btn:disabled {
      background-colour: #6c757d;
      cursor: not-allowed;
    }

    .error-message {
      colour: #dc3545;
      font-size: 0.875rem;
      margin-top: 0.25rem;
    }

    .message {
      margin-top: 1rem;
      padding: 0.75rem;
      border-radius: 4px;
      text-align: center;
    }

    .message.success {
      background-colour: #d4edda;
      colour: #155724;
      border: 1px solid #c3e6cb;
    }

    .message.error {
      background-colour: #f8d7da;
      colour: #721c24;
      border: 1px solid #f5c6cb;
    }
  `]
})
export class CreateSinkComponent implements OnInit {
  sinkForm: FormGroup;
  isCreating = false;
  message = '';
  messageType = '';

  constructor(
    private formBuilder: FormBuilder,
    private apiService: ApiService
  ) {
    this.sinkForm = this.formBuilder.group({
      name: ['', Validators.required],
      type: ['ApriltagSink', Validators.required]
    });
  }

  ngOnInit(): void {
    // Additional initialization if needed
  }

  onCreateSink(): void {
    if (this.sinkForm.invalid) {
      return;
    }

    this.isCreating = true;
    this.message = '';

    const { name, type } = this.sinkForm.value;

    this.apiService.createSink(name, type).subscribe({
      next: (sinkId) => {
        this.message = `Sink created successfully with ID: ${sinkId}`;
        this.messageType = 'success';
        this.sinkForm.reset({ name: '', type: 'ApriltagSink' });
        this.isCreating = false;
      },
      error: (error) => {
        console.error('Error creating sink:', error);
        this.message = 'Failed to create sink. Please try again.';
        this.messageType = 'error';
        this.isCreating = false;
      }
    });
  }
}