import React from 'react';
import {
  Camera,
  Plus,
  Settings2,
  Trash2,
  StopCircle,
  PlayCircle,
} from 'lucide-react';
import type { Source, Sink } from '../types';
import { WebRTCStream } from '../components/WebRTCStream';
import { BulkUploadComponent } from '../components/BulkUploadComponent';

export const SourcesPage: React.FC<{
  sources: Source[];
  sinks: Sink[];
  streamingSinks: Set<number>;
  loading: boolean;
  onAddSource: () => void;
  onConfigure:(s:Source)=>void;
  onDelete:(id:number)=>void;
  onTogglePreview:(node: {id:number; name:string})=>void;
  onStreamError:(id:number, error:string)=>void;
  onBulkVideoUpload: (files: FileList, fps?: number) => Promise<{ success: boolean; sourceIds: number[]; message: string }>;
  onBulkImageUpload: (files: FileList) => Promise<{ success: boolean; sourceIds: number[]; message: string }>;
}> = ({ sources, sinks, streamingSinks, loading, onAddSource, onConfigure, onDelete, onTogglePreview, onStreamError, onBulkVideoUpload, onBulkImageUpload }) => (
  <div className="space-y-6">
    <div className="flex justify-between items-center">
      <div>
        <h2 className="text-2xl font-bold text-gray-900 dark:text-white flex items-center gap-2"><Camera className="w-6 h-6" />Video Sources</h2>
        <p className="text-gray-600 dark:text-gray-400">Manage your video input sources</p>
      </div>
      <button onClick={onAddSource} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2"><Plus className="w-4 h-4" />Add Source</button>
    </div>

    {/* Bulk Upload Component */}
    <BulkUploadComponent
      onVideoUpload={onBulkVideoUpload}
      onImageUpload={onBulkImageUpload}
      className="mb-6"
    />

    {sources.length === 0 && !loading ? (
      <div className="text-center py-12">
        <Camera className="w-16 h-16 mx-auto mb-4 text-gray-400" />
        <h3 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">No Sources Found</h3>
        <p className="text-gray-600 dark:text-gray-400 mb-4">Add a video source to get started with processing.</p>
        <button onClick={onAddSource} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2 mx-auto"><Plus className="w-4 h-4" />Add Your First Source</button>
      </div>
    ) : (
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {sources.map(source => {
          const preview = sinks.find(s => s.type === 'webrtc' && s.sourceId === source.id);
          const isStreaming = preview != null && streamingSinks.has(preview.id);
          return (
          <div key={source.id} className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow hover:shadow-lg transition-all">
            <div className="flex justify-between items-start mb-4">
              <div className="flex items-center gap-2">
                <Camera className="w-5 h-5 text-blue-600" />
                <h3 className="text-lg font-semibold text-gray-900 dark:text-white">{source.name}</h3>
              </div>
              <div className="flex gap-2">
                <button onClick={()=>onConfigure(source)} className="px-3 py-1 bg-blue-600 text-white rounded text-xs hover:bg-blue-700 flex items-center gap-1"><Settings2 className="w-3 h-3" />Config</button>
                <button onClick={()=> { if(confirm('Delete source?')) onDelete(source.id); }} className="px-3 py-1 bg-red-600 text-white rounded text-xs hover:bg-red-700 flex items-center gap-1"><Trash2 className="w-3 h-3" />Del</button>
              </div>
            </div>
            <p className="text-gray-600 dark:text-gray-400 mb-2">Type: {source.type}</p>
            <p className="text-sm text-gray-500 dark:text-gray-500 mb-4">ID: {source.id}</p>
            {source.filePath && (
              <p className="text-xs text-gray-400 mb-2 truncate" title={source.filePath}>Path: {source.filePath}</p>
            )}
            {source.fps && (
              <p className="text-xs text-gray-400 mb-2">FPS: {source.fps}</p>
            )}
            <button onClick={()=>onTogglePreview({id: source.id, name: source.name})}
              className={`w-full px-3 py-2 rounded text-sm flex items-center gap-1 justify-center mb-3 ${isStreaming ? 'bg-red-600 hover:bg-red-700' : 'bg-green-600 hover:bg-green-700'} text-white`}>
              {isStreaming ? <><StopCircle className="w-4 h-4" />Stop Preview</> : <><PlayCircle className="w-4 h-4" />Live Preview</>}
            </button>
            {isStreaming && preview && (
              <div className="mb-3">
                <WebRTCStream sinkId={preview.id} onStop={()=>onTogglePreview({id: source.id, name: source.name})} onError={(error)=>onStreamError(preview.id, error)} />
              </div>
            )}
            <div className="flex justify-between items-center">
              <span className="text-xs text-gray-400">Updated: {source.lastUpdate?.toLocaleTimeString()}</span>
            </div>
          </div>
          );
        })}
      </div>
    )}
  </div>
);
