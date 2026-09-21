import React from 'react';
import {
  Target,
  Plus,
  Settings2,
  Trash2,
  StopCircle,
  PlayCircle,
  Cog,
  ExternalLink,
} from 'lucide-react';
import type { Sink, NT4Defaults } from '../types';
import { WebRTCStream } from '../components/WebRTCStream';
import { ToggleSwitch } from '../components/ToggleSwitch';
import { PUBLISHABLE_SINK_TYPES } from '../hooks/useAppData';

export const SinksPage: React.FC<{
  sinks: Sink[];
  loading: boolean;
  streamingSinks: Set<number>;
  nt4Settings: NT4Defaults;
  onAddSink:()=>void;
  onTogglePreview:(node: {id:number; name:string})=>void;
  onToggleNT4Publish:(node: {id:number; name:string}, nt4: NT4Defaults)=>void;
  onStreamError:(id:number, error:string)=>void;
  onConfigure:(s:Sink)=>void;
  onDelete:(id:number)=>void;
  onToggleSink:(id:number, enabled:boolean)=>void;
}> = ({ sinks, loading, streamingSinks, nt4Settings, onAddSink, onTogglePreview, onToggleNT4Publish, onStreamError, onConfigure, onDelete, onToggleSink }) => {
  // WebRTC/NetworkTables sinks are plumbing auto-created by the Live Preview / Publish to NT4
  // toggles below - they're not something the user directly created, so they don't get their
  // own card (that's the whole point of them not being addable sink types anymore).
  const visibleSinks = sinks.filter(s => s.type !== 'webrtc' && s.type !== 'networktables');
  return (
  <div className="space-y-6">
    <div className="flex justify-between items-center">
      <div>
        <h2 className="text-2xl font-bold text-gray-900 dark:text-white flex items-center gap-2"><Target className="w-6 h-6" />Processing Sinks</h2>
        <p className="text-gray-600 dark:text-gray-400">Manage your vision processing pipelines</p>
      </div>
      <button onClick={onAddSink} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2"><Plus className="w-4 h-4" />Add Sink</button>
    </div>
    {visibleSinks.length === 0 && !loading ? (
      <div className="text-center py-12">
        <Target className="w-16 h-16 mx-auto mb-4 text-gray-400" />
        <h3 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">No Sinks Found</h3>
        <p className="text-gray-600 dark:text-gray-400 mb-4">Add a processing sink to analyze video streams.</p>
        <button onClick={onAddSink} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2 mx-auto"><Plus className="w-4 h-4" />Add Your First Sink</button>
      </div>
    ) : (
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {visibleSinks.map(sink => {
          const preview = sinks.find(s => s.type === 'webrtc' && s.sourceId === sink.id);
          const isStreaming = preview != null && streamingSinks.has(preview.id);
          const nt4 = sinks.find(s => s.type === 'networktables' && s.sourceId === sink.id);
          const canPublish = PUBLISHABLE_SINK_TYPES.includes(sink.type);
          return (
          <div key={sink.id} className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow hover:shadow-lg transition-all">
            <div className="flex justify-between items-start mb-4">
              <div className="flex items-center gap-2">
                <Target className="w-5 h-5 text-green-600" />
                <h3 className="text-lg font-semibold text-gray-900 dark:text-white">{sink.name}</h3>
              </div>
              <div className="flex gap-2">
                <button onClick={()=>onConfigure(sink)} className="px-3 py-1 bg-blue-600 text-white rounded text-xs hover:bg-blue-700 flex items-center gap-1"><Settings2 className="w-3 h-3" />Config</button>
                <button onClick={()=> { if(confirm('Delete sink?')) onDelete(sink.id); }} className="px-3 py-1 bg-red-600 text-white rounded text-xs hover:bg-red-700 flex items-center gap-1"><Trash2 className="w-3 h-3" />Del</button>
              </div>
            </div>
            <p className="text-gray-600 dark:text-gray-400 mb-2">Type: {sink.type}</p>
            <p className="text-sm text-gray-500 dark:text-gray-500 mb-4">ID: {sink.id}</p>
            {sink.sourceId && (
              <p className="text-xs text-blue-600 dark:text-blue-400 mb-3 flex items-center gap-1"><ExternalLink className="w-3 h-3" />Bound to Source {sink.sourceId}</p>
            )}
            <div className="flex items-center gap-3 mb-4">
              <div className="flex items-center gap-2">
                <span className="text-sm text-gray-700 dark:text-gray-300">Status:</span>
                <ToggleSwitch
                  enabled={sink.isEnabled ?? false}
                  onChange={(enabled) => onToggleSink(sink.id, enabled)}
                />
                <span className={`text-sm font-medium ${sink.isEnabled ? 'text-green-600 dark:text-green-400' : 'text-gray-500 dark:text-gray-400'}`}>
                  {sink.isEnabled ? 'Enabled' : 'Disabled'}
                </span>
              </div>
            </div>
            {canPublish && (
              <div className="flex items-center gap-3 mb-4">
                <div className="flex items-center gap-2">
                  <span className="text-sm text-gray-700 dark:text-gray-300">Publish to NT4:</span>
                  <ToggleSwitch
                    enabled={nt4?.isEnabled ?? false}
                    onChange={() => onToggleNT4Publish({id: sink.id, name: sink.name}, nt4Settings)}
                  />
                </div>
              </div>
            )}
            <div className="flex gap-2 mb-4">
              <button onClick={()=>onTogglePreview({id: sink.id, name: sink.name})}
                className={`flex-1 px-3 py-2 rounded text-sm flex items-center gap-1 justify-center text-white ${isStreaming ? 'bg-red-600 hover:bg-red-700' : 'bg-green-600 hover:bg-green-700'}`}
                disabled={sink.status==='error'}>
                {isStreaming ? <><StopCircle className="w-4 h-4" />Stop Preview</> : <><PlayCircle className="w-4 h-4" />Live Preview</>}
              </button>
              <button onClick={()=>onConfigure(sink)} className="px-3 py-2 bg-gray-600 text-white rounded text-sm hover:bg-gray-700 transition-colors" title="Configure"><Cog className="w-4 h-4" /></button>
            </div>
            {isStreaming && preview && (
              <div className="mb-4">
                <WebRTCStream
                  sinkId={preview.id}
                  onStop={() => onTogglePreview({id: sink.id, name: sink.name})}
                  onError={(error) => onStreamError(preview.id, error)}
                />
              </div>
            )}
            <div className="text-xs text-gray-400">Updated: {sink.lastUpdate?.toLocaleTimeString()}</div>
          </div>
          );
        })}
      </div>
    )}
  </div>
  );
};
