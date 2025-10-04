import React, { useState, useEffect } from 'react';
import {
  Settings,
  Sun,
  Moon,
  RefreshCw,
  Target,
  Camera,
  BarChart3,
  Plus,
  X,
  CheckCircle,
  AlertCircle,
  Circle,
  MonitorSpeaker,
  Activity,
  Wifi,
  WifiOff,
  AlertTriangle,
  Cog,
  XCircle,
  StopCircle,
  PlayCircle,
  Settings2,
  ExternalLink,
  Trash2,
  Link2,
  Unlink
} from 'lucide-react';
import './App.css';

import type { 
  Source, 
  Sink, 
  SystemStats, 
  Settings as SettingsType
} from './types';

import { WebRTCStream } from './components/WebRTCStream';
import { AddSourceModal } from './components/AddSourceModal';
import { BulkUploadComponent } from './components/BulkUploadComponent';
import { useAppData } from './hooks/useAppData';

/***************************
 * Dark Mode Hook (original)
 ***************************/
const useDarkMode = () => {
  const [darkMode, setDarkMode] = useState(() => {
    const saved = localStorage.getItem('darkMode');
    return saved ? JSON.parse(saved) : false;
  });

  useEffect(() => {
    localStorage.setItem('darkMode', JSON.stringify(darkMode));
    if (darkMode) {
      document.documentElement.classList.add('dark');
    } else {
      document.documentElement.classList.remove('dark');
    }
  }, [darkMode]);

  return [darkMode, setDarkMode] as const;
}

/*****************
 * Toast Component
 *****************/
const Toast: React.FC<{ 
  message: string; 
  type: 'success' | 'error' | 'info'; 
  onClose: () => void;
}> = ({ message, type, onClose }) => {
  useEffect(() => {
    const timer = setTimeout(onClose, 5000);
    return () => clearTimeout(timer);
  }, [onClose]);

  const getToastcolour = () => {
    switch (type) {
      case 'success': return 'bg-green-600';
      case 'error': return 'bg-red-600';
      case 'info': return 'bg-blue-600';
      default: return 'bg-gray-600';
    }
  };

  const getToastIcon = () => {
    switch (type) {
      case 'success': return <CheckCircle className="w-4 h-4" />;
      case 'error': return <XCircle className="w-4 h-4" />;
      case 'info': return <AlertCircle className="w-4 h-4" />;
      default: return <Circle className="w-4 h-4" />;
    }
  };

  return (
    <div className={`fixed top-4 right-4 ${getToastcolour()} text-white px-4 py-2 rounded-lg shadow-lg z-50 animate-slide-up`}>
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          {getToastIcon()}
          <span>{message}</span>
        </div>
        <button onClick={onClose} className="ml-3 text-white hover:text-gray-200">
          <X className="w-4 h-4" />
        </button>
      </div>
    </div>
  );
}

/****************
 * Modal Wrapper
 ****************/
const Modal: React.FC<{ isOpen: boolean; onClose: () => void; title: string; children: React.ReactNode }> = ({ 
  isOpen, 
  onClose, 
  title, 
  children 
}) => {
  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white dark:bg-gray-800 rounded-lg p-6 w-full max-w-md mx-4">
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-xl font-bold text-gray-900 dark:text-white flex items-center gap-2">
            <Plus className="w-5 h-5" />
            {title}
          </h2>
          <button onClick={onClose} className="text-gray-500 hover:text-gray-700">
            <X className="w-5 h-5" />
          </button>
        </div>
        {children}
      </div>
    </div>
  );
}

/****************
 * Add Sink Modal
 ****************/
const AddSinkModal: React.FC<{ isOpen: boolean; onClose: () => void; onAdd: (name: string, type: string) => void; }> = ({ isOpen, onClose, onAdd }) => {
  const [name, setName] = useState('');
  const [type, setType] = useState('ApriltagSink');

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (name.trim()) {
      onAdd(name.trim(), type);
      setName('');
      setType('ApriltagSink');
      onClose();
    }
  };

  return (
    <Modal isOpen={isOpen} onClose={onClose} title="Add New Sink">
      <form onSubmit={handleSubmit} className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
            Sink Name
          </label>
            <input
              type="text"
              value={name}
              onChange={(e) => setName(e.target.value)}
              className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white"
              placeholder="Enter sink name"
              required
            />
        </div>
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
            Sink Type
          </label>
          <select
            value={type}
            onChange={(e) => setType(e.target.value)}
            className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white"
          >
            <option value="ApriltagSink">AprilTag Detection</option>
            <option value="record">Recording</option>
            <option value="stereo">Stereo Vision</option>
            <option value="calibration">Camera Calibration</option>
          </select>
        </div>
        <div className="flex justify-end space-x-3 mt-6">
          <button type="button" onClick={onClose} className="px-4 py-2 text-gray-600 dark:text-gray-400 hover:text-gray-800 dark:hover:text-gray-200">Cancel</button>
          <button type="submit" className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2">
            <Plus className="w-4 h-4" />Add Sink
          </button>
        </div>
      </form>
    </Modal>
  );
}

/*******************
 * Settings Modal
 *******************/
const SettingsModal: React.FC<{ 
  isOpen: boolean; 
  onClose: () => void; 
  settings: SettingsType; 
  onSave: (settings: SettingsType) => void;
}> = ({ isOpen, onClose, settings, onSave }) => {
  const [localSettings, setLocalSettings] = useState(settings);
  useEffect(()=> setLocalSettings(settings), [settings]);
  return (
    <Modal isOpen={isOpen} onClose={onClose} title="Settings">
      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Server URL</label>
          <input
            type="text"
            value={localSettings.serverUrl}
            onChange={(e) => setLocalSettings({...localSettings, serverUrl: e.target.value})}
            className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white"
          />
        </div>
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Refresh Interval (seconds)</label>
          <input
            type="number" min="1" max="60"
            value={localSettings.refreshInterval}
            onChange={(e) => setLocalSettings({...localSettings, refreshInterval: parseInt(e.target.value)})}
            className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white"
          />
        </div>
      </div>
      <div className="flex justify-end space-x-3 mt-6">
        <button onClick={onClose} className="px-4 py-2 text-gray-600 dark:text-gray-400 hover:text-gray-800 dark:hover:text-gray-200">Cancel</button>
        <button onClick={() => { onSave(localSettings); onClose(); }} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2">
          <CheckCircle className="w-4 h-4" />Save
        </button>
      </div>
    </Modal>
  );
}

/*******************
 * Configure Modals
 *******************/
const ConfigureSourceModal: React.FC<{ isOpen:boolean; onClose:()=>void; source: Source|null; onRename:(name:string)=>void; onDelete:()=>void; }> = ({ isOpen, onClose, source, onRename, onDelete }) => {
  const [name, setName] = useState(source?.name || '');
  useEffect(()=> setName(source?.name || ''), [source]);
  if(!isOpen || !source) return null;
  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Configure Source #${source.id}`}>
      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Name</label>
          <input value={name} onChange={e=>setName(e.target.value)} className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white" />
        </div>
      </div>
      <div className="flex justify-between mt-6">
        <button onClick={()=> { if(confirm('Delete this source?')) { onDelete(); onClose(); } }} className="px-4 py-2 bg-red-600 text-white rounded hover:bg-red-700 flex items-center gap-2">
          <Trash2 className="w-4 h-4" />Delete
        </button>
        <div className="flex gap-3">
          <button onClick={onClose} className="px-4 py-2 text-gray-600 dark:text-gray-400">Cancel</button>
          <button onClick={()=> { onRename(name); onClose(); }} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700">Save</button>
        </div>
      </div>
    </Modal>
  );
};

const ConfigureSinkModal: React.FC<{ isOpen:boolean; onClose:()=>void; sink: Sink|null; sources: Source[]; onRename:(name:string)=>void; onDelete:()=>void; onBind:(sourceId:number)=>void; onUnbind:(sourceId:number)=>void; }> = ({ isOpen, onClose, sink, sources, onRename, onDelete, onBind, onUnbind }) => {
  const [name, setName] = useState(sink?.name || '');
  const [selected, setSelected] = useState<number | ''>(sink?.sourceId ?? '');
  useEffect(()=> { setName(sink?.name || ''); setSelected(sink?.sourceId ?? ''); }, [sink]);
  if(!isOpen || !sink) return null;
  const bound = sink.sourceId != null;
  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Configure Sink #${sink.id}`}>
      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Name</label>
          <input value={name} onChange={e=>setName(e.target.value)} className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white" />
        </div>
        <div>
          <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Source Binding</label>
          <div className="flex gap-2">
            <select value={selected} onChange={e=>setSelected(e.target.value === '' ? '' : Number(e.target.value))} className="flex-1 px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md dark:bg-gray-700 dark:text-white">
              <option value="">-- None --</option>
              {sources.map(s => <option key={s.id} value={s.id}>{s.name} (ID {s.id})</option>)}
            </select>
            {bound ? (
              <button onClick={()=> { if(sink.sourceId!=null){ onUnbind(sink.sourceId); setSelected(''); } }} className="px-3 py-2 bg-yellow-600 text-white rounded hover:bg-yellow-700 flex items-center gap-1"><Unlink className="w-4 h-4" />Unbind</button>
            ) : (
              <button disabled={selected===''} onClick={()=> { if(selected!==''){ onBind(Number(selected)); } }} className="px-3 py-2 bg-green-600 disabled:opacity-50 text-white rounded hover:bg-green-700 flex items-center gap-1"><Link2 className="w-4 h-4" />Bind</button>
            )}
          </div>
        </div>
      </div>
      <div className="flex justify-between mt-6">
        <button onClick={()=> { if(confirm('Delete this sink?')) { onDelete(); onClose(); } }} className="px-4 py-2 bg-red-600 text-white rounded hover:bg-red-700 flex items-center gap-2">
          <Trash2 className="w-4 h-4" />Delete
        </button>
        <div className="flex gap-3">
          <button onClick={onClose} className="px-4 py-2 text-gray-600 dark:text-gray-400">Cancel</button>
          <button onClick={()=> { onRename(name); onClose(); }} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700">Save</button>
        </div>
      </div>
    </Modal>
  );
};

/*********************
 * System Status Card
 *********************/
const SystemStatus: React.FC<{ systemStats: SystemStats; deviceStats: any }> = ({ systemStats, deviceStats }) => (
  <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-6">
    <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg">
      <div className="flex items-center justify-between">
        <div>
          <h3 className="text-lg font-semibold text-gray-900 dark:text-white">Sources</h3>
          <p className="text-3xl font-bold text-blue-600 dark:text-blue-400">{systemStats.sources}</p>
        </div>
        <Camera className="w-10 h-10 text-blue-600" />
      </div>
    </div>
    <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg">
      <div className="flex items-center justify-between">
        <div>
          <h3 className="text-lg font-semibold text-gray-900 dark:text-white">Sinks</h3>
          <p className="text-3xl font-bold text-green-600 dark:text-green-400">{systemStats.sinks}</p>
        </div>
        <Target className="w-10 h-10 text-green-600" />
      </div>
    </div>
    <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg">
      <div className="flex items-center justify-between">
        <div>
          <h3 className="text-lg font-semibold text-gray-900 dark:text-white">CPU Usage</h3>
          <p className="text-3xl font-bold text-purple-600 dark:text-purple-400">{deviceStats.cpuUsage}%</p>
        </div>
        <Activity className="w-10 h-10 text-purple-600" />
      </div>
    </div>
    <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow transition-all hover:shadow-lg">
      <div className="flex items-center justify-between">
        <div>
          <h3 className="text-lg font-semibold text-gray-900 dark:text-white">Server Status</h3>
          <p className={`text-3xl font-bold ${
            systemStats.serverStatus === 'online' ? 'text-green-600 dark:text-green-400' :
            systemStats.serverStatus === 'offline' ? 'text-gray-600 dark:text-gray-400' :
            'text-red-600 dark:text-red-400'
          }`}>
            {systemStats.serverStatus === 'online' ? 'Online' : 
             systemStats.serverStatus === 'offline' ? 'Offline' : 'Error'}
          </p>
        </div>
        {systemStats.serverStatus === 'online' ? 
          <CheckCircle className="w-10 h-10 text-green-600" /> : 
         systemStats.serverStatus === 'offline' ? 
          <Circle className="w-10 h-10 text-gray-600" /> : 
          <XCircle className="w-10 h-10 text-red-600" />}
      </div>
    </div>
  </div>
);

/****************
 * Header & Nav
 ****************/
const Header: React.FC<{
  systemStats: SystemStats;
  darkMode: boolean;
  onToggleDarkMode: () => void;
  onShowSettings: () => void;
  onRefresh: () => void;
}> = ({ systemStats, darkMode, onToggleDarkMode, onShowSettings, onRefresh }) => (
  <header className="bg-white dark:bg-gray-800 shadow-sm border-b border-gray-200 dark:border-gray-700">
    <div className="px-6 py-4">
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-4">
          <div className="flex items-center space-x-2">
            <Target className="w-8 h-8 text-blue-600" />
            <h1 className="text-2xl font-bold text-gray-900 dark:text-white">FRCV Dashboard</h1>
          </div>
          <span className={`px-3 py-1 rounded-full text-sm font-medium flex items-center gap-1 ${
            systemStats.serverStatus === 'online' 
              ? 'bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200' 
              : systemStats.serverStatus === 'error'
              ? 'bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200'
              : 'bg-gray-100 text-gray-800 dark:bg-gray-700 dark:text-gray-300'
          }`}>
            {systemStats.serverStatus === 'online' ? 
              <>
                <Wifi className="w-4 h-4" />Connected
              </> : 
             systemStats.serverStatus === 'error' ? 
              <>
                <AlertTriangle className="w-4 h-4" />Error
              </> : 
              <>
                <WifiOff className="w-4 h-4" />Disconnected
              </>}
          </span>
        </div>
        <div className="flex items-center space-x-4">
          <button onClick={onShowSettings} className="p-2 rounded-lg bg-gray-100 dark:bg-gray-700 text-gray-600 dark:text-gray-300 hover:bg-gray-200 dark:hover:bg-gray-600" title="Settings">
            <Settings className="w-5 h-5" />
          </button>
          <button onClick={onToggleDarkMode} className="p-2 rounded-lg bg-gray-100 dark:bg-gray-700 text-gray-600 dark:text-gray-300 hover:bg-gray-200 dark:hover:bg-gray-600" title="Toggle dark mode">
            {darkMode ? <Sun className="w-5 h-5" /> : <Moon className="w-5 h-5" />}
          </button>
          <button onClick={onRefresh} className="px-3 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2" title="Refresh data">
            <RefreshCw className="w-4 h-4" />Refresh
          </button>
        </div>
      </div>
    </div>
  </header>
);

const Navigation: React.FC<{ currentTab: string; onTabChange: (tab: string) => void; streamingCount: number; }> = ({ currentTab, onTabChange, streamingCount }) => (
  <nav className="bg-white dark:bg-gray-800 border-b border-gray-200 dark:border-gray-700">
    <div className="px-6">
      <div className="flex space-x-8">
        {[{ id: 'dashboard', label: 'Dashboard', icon: BarChart3 },{ id: 'sources', label: 'Sources', icon: Camera },{ id: 'sinks', label: 'Sinks', icon: Target }].map(tab => (
          <button key={tab.id} onClick={()=>onTabChange(tab.id)} className={`flex items-center space-x-2 py-4 px-2 border-b-2 font-medium text-sm transition-colours ${currentTab===tab.id ? 'border-blue-500 text-blue-600 dark:text-blue-400' : 'border-transparent text-gray-500 hover:text-gray-700 dark:text-gray-400 dark:hover:text-gray-300'}`}>
            <tab.icon className="w-4 h-4" />
            <span>{tab.label}</span>
            {tab.id === 'dashboard' && streamingCount > 0 && (
              <span className="bg-red-500 text-white text-xs rounded-full px-2 py-0.5 min-w-[20px] h-5 flex items-center justify-center">{streamingCount}</span>
            )}
          </button>
        ))}
      </div>
    </div>
  </nav>
);

/*****************
 * Dashboard Page
 *****************/
const DashboardPage: React.FC<{
  systemStats: SystemStats;
  deviceStats: any;
  streamingSinks: Set<number>;
  sources: Source[];
  sinks: Sink[];
  onStopAllStreams: () => void;
  onStopStream: (id: number) => void;
  onStreamError: (id: number, error: string) => void;
  onStartStream: (id: number) => void;
  onGoToSinks: () => void;
  onGoToSources: () => void;
  onStartUDP: () => void;
  onStopUDP: () => void;
}> = ({ systemStats, deviceStats, streamingSinks, sources, sinks, onStopAllStreams, onStopStream, onStreamError, onStartStream, onGoToSinks, onGoToSources, onStartUDP, onStopUDP }) => (
  <div className="space-y-6">
    <SystemStatus systemStats={systemStats} deviceStats={deviceStats} />
    
    {/* Device Stats Row */}
    <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
      <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-lg font-semibold text-gray-900 dark:text-white">RAM Usage</h3>
            <p className="text-2xl font-bold text-orange-600 dark:text-orange-400">{(deviceStats.ramUsage / 1024 / 1024).toFixed(0)} MB</p>
          </div>
          <BarChart3 className="w-8 h-8 text-orange-600" />
        </div>
      </div>
      <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-lg font-semibold text-gray-900 dark:text-white">Disk Usage</h3>
            <p className="text-2xl font-bold text-red-600 dark:text-red-400">{deviceStats.diskUsage}%</p>
          </div>
          <BarChart3 className="w-8 h-8 text-red-600" />
        </div>
      </div>
      <div className="bg-white dark:bg-gray-800 p-6 rounded-lg shadow">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-lg font-semibold text-gray-900 dark:text-white">UDP Control</h3>
            <div className="flex gap-2 mt-2">
              <button onClick={onStartUDP} className="px-3 py-1 bg-green-600 text-white rounded text-sm hover:bg-green-700">Start</button>
              <button onClick={onStopUDP} className="px-3 py-1 bg-red-600 text-white rounded text-sm hover:bg-red-700">Stop</button>
            </div>
          </div>
          <Wifi className="w-8 h-8 text-blue-600" />
        </div>
      </div>
    </div>

    <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
      {/* Sources Widget */}
      <div className="bg-white dark:bg-gray-800 rounded-lg shadow p-6">
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-xl font-bold text-gray-900 dark:text-white flex items-center gap-2"><Camera className="w-5 h-5 text-blue-600" />Recent Sources</h2>
          <button onClick={onGoToSources} className="text-blue-600 hover:text-blue-700 text-sm font-medium flex items-center gap-1">View All <ExternalLink className="w-3 h-3" /></button>
        </div>
        {sources.length === 0 ? (
          <div className="text-center py-8">
            <Camera className="w-12 h-12 mx-auto mb-3 text-gray-400" />
            <p className="text-gray-500 dark:text-gray-400">No sources available</p>
            <button onClick={onGoToSources} className="mt-2 text-blue-600 hover:text-blue-700 text-sm">Add your first source</button>
          </div>
        ) : (
          <div className="space-y-3 max-h-64 overflow-y-auto">
            {sources.slice(0,4).map(source => (
              <div key={source.id} className="flex items-center justify-between p-3 bg-gray-50 dark:bg-gray-700 rounded-lg">
                <div className="flex items-center gap-3">
                  <div className={`w-3 h-3 rounded-full ${source.status==='active' ? 'bg-green-500' : source.status==='inactive' ? 'bg-gray-400' : 'bg-red-500'}`} />
                  <div>
                    <p className="font-medium text-gray-900 dark:text-white">{source.name}</p>
                    <p className="text-xs text-gray-500 dark:text-gray-400">{source.type}</p>
                  </div>
                </div>
                <div className="text-xs text-gray-400">ID: {source.id}</div>
              </div>
            ))}
            {sources.length > 4 && (
              <div className="text-center pt-2">
                <button onClick={onGoToSources} className="text-blue-600 hover:text-blue-700 text-sm">+{sources.length - 4} more sources</button>
              </div>
            )}
          </div>
        )}
      </div>
      {/* Sinks Widget */}
      <div className="bg-white dark:bg-gray-800 rounded-lg shadow p-6">
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-xl font-bold text-gray-900 dark:text-white flex items-center gap-2"><Target className="w-5 h-5 text-green-600" />Processing Sinks</h2>
          <button onClick={onGoToSinks} className="text-blue-600 hover:text-blue-700 text-sm font-medium flex items-center gap-1">View All <ExternalLink className="w-3 h-3" /></button>
        </div>
        {sinks.length === 0 ? (
          <div className="text-center py-8">
            <Target className="w-12 h-12 mx-auto mb-3 text-gray-400" />
            <p className="text-gray-500 dark:text-gray-400">No sinks available</p>
            <button onClick={onGoToSinks} className="mt-2 text-blue-600 hover:text-blue-700 text-sm">Add your first sink</button>
          </div>
        ) : (
          <div className="space-y-3 max-h-64 overflow-y-auto">
            {sinks.slice(0,4).map(sink => (
              <div key={sink.id} className="flex items-center justify-between p-3 bg-gray-50 dark:bg-gray-700 rounded-lg">
                <div className="flex items-center gap-3">
                  <div className={`w-3 h-3 rounded-full ${sink.status==='active' ? 'bg-green-500' : sink.status==='inactive' ? 'bg-gray-400' : 'bg-red-500'}`} />
                  <div>
                    <p className="font-medium text-gray-900 dark:text-white">{sink.name}</p>
                    <p className="text-xs text-gray-500 dark:text-gray-400">{sink.type}</p>
                  </div>
                </div>
                <div className="flex items-center gap-2">
                  {streamingSinks.has(sink.id) ? (
                    <>
                      <span className="px-2 py-1 bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200 rounded text-xs font-medium">LIVE</span>
                      <button onClick={()=>onStopStream(sink.id)} className="p-1 text-red-600 hover:text-red-700" title="Stop stream"><StopCircle className="w-4 h-4" /></button>
                    </>
                  ) : (
                    <button onClick={()=>onStartStream(sink.id)} className="p-1 text-green-600 hover:text-green-700" title="Start stream" disabled={sink.status==='error'}><PlayCircle className="w-4 h-4" /></button>
                  )}
                </div>
              </div>
            ))}
            {sinks.length > 4 && (
              <div className="text-center pt-2">
                <button onClick={onGoToSinks} className="text-blue-600 hover:text-blue-700 text-sm">+{sinks.length - 4} more sinks</button>
              </div>
            )}
          </div>
        )}
      </div>
    </div>

    {streamingSinks.size > 0 && (
      <div className="space-y-4">
        <div className="flex items-center justify-between">
          <h2 className="text-xl font-bold text-gray-900 dark:text-white flex items-center gap-2"><Activity className="w-5 h-5" />Live Streams</h2>
          <button onClick={onStopAllStreams} className="px-3 py-1 bg-red-600 text-white rounded text-sm hover:bg-red-700 flex items-center gap-1" title="Stop all streams"><StopCircle className="w-4 h-4" />Stop All Streams</button>
        </div>
        <div className="text-center py-8">
          <p className="text-gray-600 dark:text-gray-400">WebRTC streaming functionality is not yet implemented in the current API.</p>
        </div>
      </div>
    )}
    {streamingSinks.size === 0 && (
      <div className="text-center py-12">
        <MonitorSpeaker className="w-16 h-16 mx-auto mb-4 text-gray-400" />
        <h3 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">No Active Streams</h3>
        <p className="text-gray-600 dark:text-gray-400 mb-4">WebRTC streaming will be available when implemented in the API.</p>
        <button onClick={onGoToSinks} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2 mx-auto"><Target className="w-4 h-4" />Go to Sinks</button>
      </div>
    )}
  </div>
);

/****************
 * Sources Page
 ****************/
const SourcesPage: React.FC<{ 
  sources: Source[]; 
  loading: boolean; 
  onAddSource: () => void; 
  onConfigure:(s:Source)=>void; 
  onDelete:(id:number)=>void;
  onBulkVideoUpload: (files: FileList, fps?: number) => Promise<{ success: boolean; sourceIds: number[]; message: string }>;
  onBulkImageUpload: (files: FileList) => Promise<{ success: boolean; sourceIds: number[]; message: string }>;
}> = ({ sources, loading, onAddSource, onConfigure, onDelete, onBulkVideoUpload, onBulkImageUpload }) => (
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
        {sources.map(source => (
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
            <div className="flex justify-between items-center">
              <span className="text-xs text-gray-400">Updated: {source.lastUpdate?.toLocaleTimeString()}</span>
            </div>
          </div>
        ))}
      </div>
    )}
  </div>
);

/***************
 * Sinks Page
 ***************/
const SinksPage: React.FC<{ sinks: Sink[]; loading: boolean; streamingSinks: Set<number>; onAddSink:()=>void; onStartStream:(id:number)=>void; onStopStream:(id:number)=>void; onConfigure:(s:Sink)=>void; onDelete:(id:number)=>void; }> = ({ sinks, loading, streamingSinks, onAddSink, onStartStream, onStopStream, onConfigure, onDelete }) => (
  <div className="space-y-6">
    <div className="flex justify-between items-center">
      <div>
        <h2 className="text-2xl font-bold text-gray-900 dark:text-white flex items-center gap-2"><Target className="w-6 h-6" />Processing Sinks</h2>
        <p className="text-gray-600 dark:text-gray-400">Manage your vision processing pipelines</p>
      </div>
      <button onClick={onAddSink} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2"><Plus className="w-4 h-4" />Add Sink</button>
    </div>
    {sinks.length === 0 && !loading ? (
      <div className="text-center py-12">
        <Target className="w-16 h-16 mx-auto mb-4 text-gray-400" />
        <h3 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">No Sinks Found</h3>
        <p className="text-gray-600 dark:text-gray-400 mb-4">Add a processing sink to analyze video streams.</p>
        <button onClick={onAddSink} className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 flex items-center gap-2 mx-auto"><Plus className="w-4 h-4" />Add Your First Sink</button>
      </div>
    ) : (
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {sinks.map(sink => (
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
            <div className="flex gap-2 mb-4">
              {streamingSinks.has(sink.id) ? (
                <button onClick={()=>onStopStream(sink.id)} className="flex-1 px-3 py-2 bg-red-600 text-white rounded text-sm hover:bg-red-700 flex items-center gap-1 justify-center"><StopCircle className="w-4 h-4" />Stop Stream</button>
              ) : (
                <button onClick={()=>onStartStream(sink.id)} className="flex-1 px-3 py-2 bg-green-600 text-white rounded text-sm hover:bg-green-700 flex items-center gap-1 justify-center" disabled={sink.status==='error'}><PlayCircle className="w-4 h-4" />Start Stream</button>
              )}
              <button onClick={()=>onConfigure(sink)} className="px-3 py-2 bg-gray-600 text-white rounded text-sm hover:bg-gray-700 transition-colours" title="Configure"><Cog className="w-4 h-4" /></button>
            </div>
            <div className="text-xs text-gray-400">Updated: {sink.lastUpdate?.toLocaleTimeString()}</div>
          </div>
        ))}
      </div>
    )}
  </div>
);

/*******************
 * Main App
 *******************/
function App() {
  const [darkMode, setDarkMode] = useDarkMode();
  const [currentTab, setCurrentTab] = useState('dashboard');
  const [showSettings, setShowSettings] = useState(false);
  const [showAddSource, setShowAddSource] = useState(false);
  const [showAddSink, setShowAddSink] = useState(false);
  const [settings, setSettings] = useState<SettingsType>({ serverUrl: 'http://localhost:8175', refreshInterval: 5 });
  const [cfgSource, setCfgSource] = useState<Source|null>(null);
  const [cfgSink, setCfgSink] = useState<Sink|null>(null);

  const {
    sources,
    sinks,
    streamingSinks,
    loading,
    error,
    systemStats,
    deviceStats,
    toast,
    loadData,
    startStream,
    stopStream,
    handleStreamError,
    showToast,
    handleAddSource,
    handleAddSink,
    handleRenameSource,
    handleDeleteSource,
    handleRenameSink,
    handleDeleteSink,
    handleBindSink,
    handleUnbindSink,
    handleBulkVideoUpload,
    handleBulkImageUpload,
    startUDPTransmission,
    stopUDPTransmission,
    setStreamingSinks,
    setError,
    setToast
  } = useAppData();

  useEffect(() => {
    loadData();
    const interval = setInterval(loadData, settings.refreshInterval * 1000);
    return () => clearInterval(interval);
  }, [loadData, settings.refreshInterval]);

  const toggleDarkMode = () => {
    setDarkMode(!darkMode);
    showToast('Theme updated successfully', 'success');
  };

  if (loading) {
    return (
      <div className="min-h-screen bg-gray-50 dark:bg-gray-900 flex items-center justify-center">
        <div className="text-center">
          <div className="animate-spin rounded-full h-32 w-32 border-b-2 border-blue-600 mx-auto mb-4">
            <RefreshCw className="w-32 h-32 text-blue-600" />
          </div>
          <p className="text-gray-600 dark:text-gray-400 text-lg">Loading FRCV Dashboard...</p>
          <p className="text-gray-500 dark:text-gray-500 text-sm mt-2">Connecting to {settings.serverUrl}</p>
        </div>
      </div>
    );
  }

  return (
    <div className={`min-h-screen transition-colours duration-200 ${darkMode ? 'dark' : ''}`}>
      <div className="min-h-screen bg-gray-50 dark:bg-gray-900">
        <Header systemStats={systemStats} darkMode={darkMode} onToggleDarkMode={toggleDarkMode} onShowSettings={()=>setShowSettings(true)} onRefresh={loadData} />
        <Navigation currentTab={currentTab} onTabChange={setCurrentTab} streamingCount={streamingSinks.size} />

        {error && (
          <div className="bg-red-100 dark:bg-red-900 border-l-4 border-red-500 p-4 mx-6 mt-4 rounded">
            <div className="flex">
              <div className="flex-shrink-0"><AlertTriangle className="w-5 h-5 text-red-500" /></div>
              <div className="ml-3"><p className="text-sm text-red-700 dark:text-red-200">Connection Error: {error}</p></div>
              <div className="ml-auto pl-3">
                <button onClick={()=>setError(null)} className="text-red-500 hover:text-red-700"><X className="w-4 h-4" /></button>
              </div>
            </div>
          </div>
        )}

        <main className="p-6">
          {currentTab === 'dashboard' && (
            <DashboardPage 
              systemStats={systemStats} 
              deviceStats={deviceStats}
              streamingSinks={streamingSinks} 
              sources={sources} 
              sinks={sinks} 
              onStopAllStreams={()=>setStreamingSinks(new Set())} 
              onStopStream={stopStream} 
              onStreamError={handleStreamError} 
              onStartStream={startStream} 
              onGoToSinks={()=>setCurrentTab('sinks')} 
              onGoToSources={()=>setCurrentTab('sources')}
              onStartUDP={startUDPTransmission}
              onStopUDP={stopUDPTransmission}
            />
          )}
          {currentTab === 'sources' && (
            <SourcesPage 
              sources={sources} 
              loading={loading} 
              onAddSource={()=>setShowAddSource(true)} 
              onConfigure={s=>setCfgSource(s)} 
              onDelete={id=>handleDeleteSource(id)}
              onBulkVideoUpload={handleBulkVideoUpload}
              onBulkImageUpload={handleBulkImageUpload}
            />
          )}
          {currentTab === 'sinks' && (
            <SinksPage sinks={sinks} loading={loading} streamingSinks={streamingSinks} onAddSink={()=>setShowAddSink(true)} onStartStream={startStream} onStopStream={stopStream} onConfigure={s=>setCfgSink(s)} onDelete={id=>handleDeleteSink(id)} />
          )}
        </main>

        <SettingsModal isOpen={showSettings} onClose={()=>setShowSettings(false)} settings={settings} onSave={setSettings} />
        <AddSourceModal isOpen={showAddSource} onClose={()=>setShowAddSource(false)} onAdd={handleAddSource} />
        <AddSinkModal isOpen={showAddSink} onClose={()=>setShowAddSink(false)} onAdd={handleAddSink} />

        <ConfigureSourceModal isOpen={!!cfgSource} source={cfgSource} onClose={()=>setCfgSource(null)} onRename={name=> cfgSource && handleRenameSource(cfgSource.id, name)} onDelete={()=> { if(cfgSource){ handleDeleteSource(cfgSource.id); setCfgSource(null);} }} />
        <ConfigureSinkModal isOpen={!!cfgSink} sink={cfgSink} sources={sources} onClose={()=>setCfgSink(null)} onRename={name=> cfgSink && handleRenameSink(cfgSink.id, name)} onDelete={()=> { if(cfgSink){ handleDeleteSink(cfgSink.id); setCfgSink(null);} }} onBind={(src)=> cfgSink && handleBindSink(cfgSink.id, src)} onUnbind={(src)=> cfgSink && handleUnbindSink(cfgSink.id, src)} />

        {toast && <Toast message={toast.message} type={toast.type} onClose={()=>setToast(null)} />}
      </div>
    </div>
  );
}

export default App;
