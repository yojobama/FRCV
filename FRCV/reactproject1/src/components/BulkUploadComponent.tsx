import React, { useState, useRef, useCallback } from 'react';
import { Upload, X, AlertCircle, CheckCircle, File, Video, Image } from 'lucide-react';

interface BulkUploadComponentProps {
  onVideoUpload: (files: FileList, fps?: number) => Promise<{ success: boolean; sourceIds: number[]; message: string }>;
  onImageUpload: (files: FileList) => Promise<{ success: boolean; sourceIds: number[]; message: string }>;
  className?: string;
}

export const BulkUploadComponent: React.FC<BulkUploadComponentProps> = ({
  onVideoUpload,
  onImageUpload,
  className = ''
}) => {
  const [isDragOver, setIsDragOver] = useState(false);
  const [uploadProgress, setUploadProgress] = useState<{ [key: string]: number }>({});
  const [uploadResults, setUploadResults] = useState<string[]>([]);
  const [isUploading, setIsUploading] = useState(false);
  const fileInputRef = useRef<HTMLInputElement>(null);

  const validateAndCategorizeFiles = (files: FileList) => {
    const videoFiles: File[] = [];
    const imageFiles: File[] = [];
    const invalidFiles: { file: File; reason: string }[] = [];

    Array.from(files).forEach(file => {
      if (file.type.startsWith('video/')) {
        const validVideoTypes = ['video/mp4', 'video/avi', 'video/mov', 'video/wmv', 'video/mkv', 'video/webm'];
        if (validVideoTypes.includes(file.type) && file.size <= 500 * 1024 * 1024) {
          videoFiles.push(file);
        } else {
          invalidFiles.push({
            file,
            reason: file.size > 500 * 1024 * 1024 ? 'File too large (max 500MB)' : 'Unsupported video format'
          });
        }
      } else if (file.type.startsWith('image/')) {
        const validImageTypes = ['image/jpeg', 'image/jpg', 'image/png', 'image/bmp', 'image/gif', 'image/webp'];
        if (validImageTypes.includes(file.type) && file.size <= 50 * 1024 * 1024) {
          imageFiles.push(file);
        } else {
          invalidFiles.push({
            file,
            reason: file.size > 50 * 1024 * 1024 ? 'File too large (max 50MB)' : 'Unsupported image format'
          });
        }
      } else {
        invalidFiles.push({
          file,
          reason: 'Unsupported file type. Only video and image files are supported.'
        });
      }
    });

    return { videoFiles, imageFiles, invalidFiles };
  };

  const processUploads = async (files: FileList) => {
    setIsUploading(true);
    setUploadResults([]);

    const { videoFiles, imageFiles, invalidFiles } = validateAndCategorizeFiles(files);
    const results: string[] = [];

    // Report invalid files
    invalidFiles.forEach(({ file, reason }) => {
      results.push(`? ${file.name}: ${reason}`);
    });

    try {
      // Upload video files
      if (videoFiles.length > 0) {
        const videoFileList = new DataTransfer();
        videoFiles.forEach(file => videoFileList.items.add(file));
        
        setUploadProgress({ videos: 50 });
        const videoResult = await onVideoUpload(videoFileList.files, 30);
        
        if (videoResult.success) {
          results.push(`? Videos: ${videoResult.message}`);
        } else {
          results.push(`? Videos: ${videoResult.message}`);
        }
        setUploadProgress({ videos: 100 });
      }

      // Upload image files
      if (imageFiles.length > 0) {
        const imageFileList = new DataTransfer();
        imageFiles.forEach(file => imageFileList.items.add(file));
        
        setUploadProgress(prev => ({ ...prev, images: 50 }));
        const imageResult = await onImageUpload(imageFileList.files);
        
        if (imageResult.success) {
          results.push(`? Images: ${imageResult.message}`);
        } else {
          results.push(`? Images: ${imageResult.message}`);
        }
        setUploadProgress(prev => ({ ...prev, images: 100 }));
      }

    } catch (error) {
      results.push(`? Upload failed: ${error}`);
    }

    setUploadResults(results);
    setIsUploading(false);
    setTimeout(() => {
      setUploadProgress({});
      setUploadResults([]);
    }, 5000);
  };

  const handleDragEnter = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragOver(true);
  }, []);

  const handleDragLeave = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragOver(false);
  }, []);

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
  }, []);

  const handleDrop = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragOver(false);

    const files = e.dataTransfer.files;
    if (files.length > 0) {
      processUploads(files);
    }
  }, []);

  const handleFileSelect = (e: React.ChangeEvent<HTMLInputElement>) => {
    const files = e.target.files;
    if (files && files.length > 0) {
      processUploads(files);
    }
    // Reset input value to allow selecting the same files again
    e.target.value = '';
  };

  const clearResults = () => {
    setUploadResults([]);
    setUploadProgress({});
  };

  return (
    <div className={`space-y-4 ${className}`}>
      {/* Drop Zone */}
      <div
        className={`border-2 border-dashed rounded-lg p-8 text-center transition-colours ${
          isDragOver
            ? 'border-blue-400 bg-blue-50 dark:bg-blue-900/20'
            : 'border-gray-300 dark:border-gray-600 hover:border-gray-400 dark:hover:border-gray-500'
        } ${isUploading ? 'opacity-50 pointer-events-none' : ''}`}
        onDragEnter={handleDragEnter}
        onDragLeave={handleDragLeave}
        onDragOver={handleDragOver}
        onDrop={handleDrop}
      >
        <div className="space-y-4">
          <div className="flex justify-center">
            <Upload className={`w-12 h-12 ${isDragOver ? 'text-blue-500' : 'text-gray-400'}`} />
          </div>
          
          <div>
            <h3 className="text-lg font-medium text-gray-900 dark:text-white">
              {isDragOver ? 'Drop files here' : 'Bulk File Upload'}
            </h3>
            <p className="text-sm text-gray-600 dark:text-gray-400">
              Drag and drop video and image files here, or click to select files
            </p>
          </div>

          <div className="flex justify-center space-x-6 text-xs text-gray-500 dark:text-gray-400">
            <div className="flex items-center gap-1">
              <Video className="w-4 h-4" />
              <span>Videos: MP4, AVI, MOV, WMV, MKV, WebM (max 500MB)</span>
            </div>
            <div className="flex items-center gap-1">
              <Image className="w-4 h-4" />
              <span>Images: JPEG, PNG, BMP, GIF, WebP (max 50MB)</span>
            </div>
          </div>

          <button
            type="button"
            onClick={() => fileInputRef.current?.click()}
            disabled={isUploading}
            className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 disabled:bg-gray-400 disabled:cursor-not-allowed transition-colours"
          >
            {isUploading ? 'Uploading...' : 'Select Files'}
          </button>

          <input
            ref={fileInputRef}
            type="file"
            multiple
            accept="video/*,image/*"
            onChange={handleFileSelect}
            className="hidden"
          />
        </div>
      </div>

      {/* Upload Progress */}
      {Object.keys(uploadProgress).length > 0 && (
        <div className="space-y-2">
          {Object.entries(uploadProgress).map(([key, progress]) => (
            <div key={key} className="space-y-1">
              <div className="flex justify-between text-sm">
                <span className="capitalize text-gray-700 dark:text-gray-300">{key}</span>
                <span className="text-gray-500 dark:text-gray-400">{progress}%</span>
              </div>
              <div className="w-full bg-gray-200 dark:bg-gray-700 rounded-full h-2">
                <div
                  className="bg-blue-600 h-2 rounded-full transition-all duration-300"
                  style={{ width: `${progress}%` }}
                />
              </div>
            </div>
          ))}
        </div>
      )}

      {/* Upload Results */}
      {uploadResults.length > 0 && (
        <div className="bg-gray-50 dark:bg-gray-800 rounded-lg p-4">
          <div className="flex items-center justify-between mb-3">
            <h4 className="text-sm font-medium text-gray-900 dark:text-white">Upload Results</h4>
            <button
              onClick={clearResults}
              className="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300"
            >
              <X className="w-4 h-4" />
            </button>
          </div>
          <div className="space-y-2">
            {uploadResults.map((result, index) => (
              <div
                key={index}
                className="text-sm font-mono text-gray-700 dark:text-gray-300 break-words"
              >
                {result}
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
};