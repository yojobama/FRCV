import React, { useState, useEffect } from 'react';
import { Trash2, Link2, Unlink } from 'lucide-react';
import { Modal } from './Modal';
import { ToggleSwitch } from './ToggleSwitch';
import type { Source, Sink } from '../types';

export const ConfigureSourceModal: React.FC<{ isOpen: boolean; onClose: () => void; source: Source | null; onRename: (name: string) => void; onDelete: () => void; }> = ({ isOpen, onClose, source, onRename, onDelete }) => {
  const [name, setName] = useState(source?.name || '');
  useEffect(() => setName(source?.name || ''), [source]);
  if (!isOpen || !source) return null;
  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Configure Source #${source.id}`}>
      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Name</label>
          <input value={name} onChange={e => setName(e.target.value)} className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white" />
        </div>
      </div>
      <div className="flex justify-between mt-6">
        <button onClick={() => { if (confirm('Delete this source?')) { onDelete(); onClose(); } }} className="px-4 py-2 bg-red-600 text-white rounded hover:bg-red-700 flex items-center gap-2">
          <Trash2 className="w-4 h-4" />Delete
        </button>
        <div className="flex gap-3">
          <button onClick={onClose} className="px-4 py-2 text-gray-600 dark:text-gray-400">Cancel</button>
          <button onClick={() => { onRename(name); onClose(); }} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700">Save</button>
        </div>
      </div>
    </Modal>
  );
};

export const ConfigureSinkModal: React.FC<{ isOpen: boolean; onClose: () => void; sink: Sink | null; sources: Source[]; onRename: (name: string) => void; onDelete: () => void; onBind: (sourceId: number) => void; onUnbind: (sourceId: number) => void; }> = ({ isOpen, onClose, sink, sources, onRename, onDelete, onBind, onUnbind }) => {
  const [name, setName] = useState(sink?.name || '');
  const [selected, setSelected] = useState<number | ''>(sink?.sourceId ?? '');
  const [enabled, setEnabled] = useState(sink?.isEnabled ?? false);
  useEffect(() => { setName(sink?.name || ''); setSelected(sink?.sourceId ?? ''); setEnabled(sink?.isEnabled ?? false); }, [sink]);
  if (!isOpen || !sink) return null;
  const bound = sink.sourceId != null;
  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Configure Sink #${sink.id}`}>
      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Name</label>
          <input value={name} onChange={e => setName(e.target.value)} className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white" />
        </div>
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Source Binding</label>
          <div className="flex gap-2">
            <select value={selected} onChange={e => setSelected(e.target.value === '' ? '' : Number(e.target.value))} className="flex-1 px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white">
              <option value="">-- None --</option>
              {sources.map(s => <option key={s.id} value={s.id}>{s.name} (ID {s.id})</option>)}
            </select>
            {bound ? (
              <button onClick={() => { if (sink.sourceId != null) { onUnbind(sink.sourceId); setSelected(''); } }} className="px-3 py-2 bg-yellow-600 text-white rounded hover:bg-yellow-700 flex items-center gap-1"><Unlink className="w-4 h-4" />Unbind</button>
            ) : (
              <button disabled={selected === ''} onClick={() => { if (selected !== '') { onBind(Number(selected)); } }} className="px-3 py-2 bg-green-600 disabled:opacity-50 text-white rounded hover:bg-green-700 flex items-center gap-1"><Link2 className="w-4 h-4" />Bind</button>
            )}
          </div>
        </div>
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Enabled</label>
          <ToggleSwitch enabled={enabled} onChange={setEnabled} />
        </div>
      </div>
      <div className="flex justify-between mt-6">
        <button onClick={() => { if (confirm('Delete this sink?')) { onDelete(); onClose(); } }} className="px-4 py-2 bg-red-600 text-white rounded hover:bg-red-700 flex items-center gap-2">
          <Trash2 className="w-4 h-4" />Delete
        </button>
        <div className="flex gap-3">
          <button onClick={onClose} className="px-4 py-2 text-gray-600 dark:text-gray-400">Cancel</button>
          <button onClick={() => { onRename(name); onClose(); }} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700">Save</button>
        </div>
      </div>
    </Modal>
  );
};
