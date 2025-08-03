import React, { useState, useEffect } from 'react';
import { Plus, X, Upload, Camera, Video, Image } from 'lucide-react';
import { CameraHardwareInfo } from '../types';

interface AddSourceModalProps {
  isOpen: boolean;
  onClose: () => void;
  onAdd: (name: string, type: string, file?: File, fps?: number, hardwareInfo?: CameraHardwareInfo) => void;
}

export const AddSourceModal: React.FC<AddSourceModalProps> = ({ 
  isOpen, 
  onClose, 
  onAdd 
}) => {
  const [name, setName] = useState('');
  const [type, setType] = useState('camera');
  const [file, setFile] = useState<File | null>(null);
  const [fps, setFps] = useState(30);
  const [selectedCamera, setSelectedCamera] = useState<CameraHardwareInfo | null>(null);
  const [cameras, setCameras] = useState<CameraHardwareInfo[]>([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (isOpen && type === 'camera') {
      loadCameras();
    }
  }, [isOpen, type]);

  const loadCameras = async () => {
    try {
      setLoading(true);
      const response = await fetch('http://localhost:8175/api/source/camera/hardwareInfos');
      if (response.ok) {
        const cameraList = await response.json();
        setCameras(cameraList);
        if (cameraList.length > 0) {
          setSelectedCamera(cameraList[0]);
        }
      }
    } catch (error) {
      console.error('Failed to load cameras:', error);
    } finally {
      setLoading(false);
    }
  };

  if (!isOpen) return null;

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!name.trim()) return;

    if (type === 'camera' && selectedCamera) {
      onAdd(name.trim(), type, undefined, undefined, selectedCamera);
    } else if ((type === 'video' || type === 'image') && file) {
      onAdd(name.trim(), type, file, type === 'video' ? fps : undefined);
    }
    
    // Reset form
    setName('');
    setType('camera');
    setFile(null);
    setFps(30);
    setSelectedCamera(null);
    onClose();
  };

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const selectedFile = e.target.files?.[0];
    if (selectedFile) {
      setFile(selectedFile);
      if (!name) {
        const fileName = selectedFile.name.split('.')[0];
        setName(fileName);
      }
    }
  };

  const getTypeIcon = () => {
    switch (type) {
      case 'camera': return <Camera className="w-5 h-5" />;
      case 'video': return <Video className="w-5 h-5" />;
      case 'image': return <Image className="w-5 h-5" />;
      default: return <Plus className="w-5 h-5" />;
    }
  };

  const canSubmit = () => {
    if (!name.trim()) return false;
    
    if (type === 'camera') {
      return selectedCamera !== null;
    } else if (type === 'video' || type === 'image') {
      return file !== null;
    }
    
    return false;
  };

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white dark:bg-gray-800 rounded-lg p-6 w-full max-w-md mx-4 max-h-[90vh] overflow-y-auto">
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-xl font-bold text-gray-900 dark:text-white flex items-center gap-2">
            {getTypeIcon()}
            Add New Source
          </h2>
          <button onClick={onClose} className="text-gray-500 hover:text-gray-700 dark:hover:text-gray-300">
            <X className="w-5 h-5" />
          </button>
        </div>
        
        <form onSubmit={handleSubmit} className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
              Source Name
            </label>
            <input
              type="text"
              value={name}
              onChange={(e) => setName(e.target.value)}
              className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white focus:outline-none focus:ring-2 focus:ring-blue-500"
              placeholder="Enter source name"
              required
            />
          </div>
          
          <div>
            <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
              Source Type
            </label>
            <select
              value={type}
              onChange={(e) => setType(e.target.value)}
              className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white focus:outline-none focus:ring-2 focus:ring-blue-500"
            >
              <option value="camera">Camera</option>
              <option value="video">Video File</option>
              <option value="image">Image File</option>
            </select>
          </div>

          {/* Camera Selection */}
          {type === 'camera' && (
            <div>
              <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
                Camera Device
              </label>
              {loading ? (
                <div className="text-sm text-gray-500 dark:text-gray-400">Loading cameras...</div>
              ) : cameras.length > 0 ? (
                <select
                  value={selectedCamera?.path || ''}
                  onChange={(e) => {
                    const camera = cameras.find(c => c.path === e.target.value);
                    setSelectedCamera(camera || null);
                  }}
                  className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white focus:outline-none focus:ring-2 focus:ring-blue-500"
                  required
                >
                  {cameras.map((camera, index) => (
                    <option key={index} value={camera.path}>
                      {camera.name} ({camera.path})
                    </option>
                  ))}
                </select>
              ) : (
                <div className="text-sm text-red-500 dark:text-red-400">No cameras found</div>
              )}
            </div>
          )}

          {/* File Upload */}
          {(type === 'video' || type === 'image') && (
            <div>
              <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
                {type === 'video' ? 'Video File' : 'Image File'}
              </label>
              <div className="relative">
                <input
                  type="file"
                  onChange={handleFileChange}
                  accept={type === 'video' ? 'video/*' : 'image/*'}
                  className="hidden"
                  id="file-upload"
                  required
                />
                <label
                  htmlFor="file-upload"
                  className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white cursor-pointer flex items-center gap-2 hover:bg-gray-50 dark:hover:bg-gray-600 transition-colors"
                >
                  <Upload className="w-4 h-4" />
                  {file ? file.name : `Choose ${type} file...`}
                </label>
              </div>
              {file && (
                <div className="text-xs text-gray-500 dark:text-gray-400 mt-1">
                  Selected: {file.name} ({(file.size / 1024 / 1024).toFixed(2)} MB)
                </div>
              )}
            </div>
          )}

          {/* FPS for video files */}
          {type === 'video' && (
            <div>
              <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
                Frame Rate (FPS)
              </label>
              <input
                type="number"
                value={fps}
                onChange={(e) => setFps(parseInt(e.target.value) || 30)}
                min="1"
                max="120"
                className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white focus:outline-none focus:ring-2 focus:ring-blue-500"
              />
              <div className="text-xs text-gray-500 dark:text-gray-400 mt-1">
                Recommended: 30 FPS for most videos
              </div>
            </div>
          )}
          
          <div className="flex justify-end space-x-3 mt-6">
            <button
              type="button"
              onClick={onClose}
              className="px-4 py-2 text-gray-600 dark:text-gray-400 hover:text-gray-800 dark:hover:text-gray-200 transition-colors"
            >
              Cancel
            </button>
            <button
              type="submit"
              disabled={!canSubmit()}
              className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 disabled:bg-gray-400 disabled:cursor-not-allowed flex items-center gap-2 transition-colors"
            >
              <Plus className="w-4 h-4" />
              Add Source
            </button>
          </div>
        </form>
      </div>
    </div>
  );
};