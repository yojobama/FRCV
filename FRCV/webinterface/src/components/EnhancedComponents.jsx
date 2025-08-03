import React, { useState, useEffect } from 'react';

// Bind/Unbind Component for managing source-sink relationships
function BindingSinkSource({ sinks, sources, onRefresh }) {
  const [selectedSink, setSelectedSink] = useState('');
  const [selectedSource, setSelectedSource] = useState('');
  const [isBinding, setIsBinding] = useState(false);

  const handleBind = async () => {
    if (!selectedSink || !selectedSource) {
      alert('Please select both a sink and a source');
      return;
    }

    setIsBinding(true);
    try {
      const response = await fetch(`http://localhost:8080/api/sink/bind?sourceId=${selectedSource}&sinkId=${selectedSink}`, {
        method: 'PUT'
      });
      
      if (!response.ok) {
        throw new Error('Failed to bind sink to source');
      }
      
      alert('Sink bound to source successfully');
      setSelectedSink('');
      setSelectedSource('');
      onRefresh && onRefresh();
    } catch (error) {
      console.error('Error binding sink to source:', error);
      alert('Failed to bind sink to source');
    } finally {
      setIsBinding(false);
    }
  };

  const handleUnbind = async () => {
    if (!selectedSink || !selectedSource) {
      alert('Please select both a sink and a source');
      return;
    }

    setIsBinding(true);
    try {
      const response = await fetch(`http://localhost:8080/api/sink/unbind?sinkId=${selectedSink}&sourceId=${selectedSource}`, {
        method: 'PUT'
      });
      
      if (!response.ok) {
        throw new Error('Failed to unbind sink from source');
      }
      
      alert('Sink unbound from source successfully');
      setSelectedSink('');
      setSelectedSource('');
      onRefresh && onRefresh();
    } catch (error) {
      console.error('Error unbinding sink from source:', error);
      alert('Failed to unbind sink from source');
    } finally {
      setIsBinding(false);
    }
  };

  return (
    <div className="binding-component">
      <h3>Bind/Unbind Sink ? Source</h3>
      
      <div className="binding-form">
        <div className="form-row">
          <div className="form-group">
            <label>Select Sink:</label>
            <select 
              value={selectedSink} 
              onChange={(e) => setSelectedSink(e.target.value)}
            >
              <option value="">Choose a sink...</option>
              {sinks.map(sinkId => (
                <option key={sinkId} value={sinkId}>
                  Sink ID: {sinkId}
                </option>
              ))}
            </select>
          </div>

          <div className="form-group">
            <label>Select Source:</label>
            <select 
              value={selectedSource} 
              onChange={(e) => setSelectedSource(e.target.value)}
            >
              <option value="">Choose a source...</option>
              {sources.map(sourceId => (
                <option key={sourceId} value={sourceId}>
                  Source ID: {sourceId}
                </option>
              ))}
            </select>
          </div>
        </div>

        <div className="button-group">
          <button 
            onClick={handleBind} 
            disabled={isBinding || !selectedSink || !selectedSource}
            className="btn btn-success"
          >
            {isBinding ? 'Binding...' : 'Bind'}
          </button>
          
          <button 
            onClick={handleUnbind} 
            disabled={isBinding || !selectedSink || !selectedSource}
            className="btn btn-warning"
          >
            {isBinding ? 'Unbinding...' : 'Unbind'}
          </button>
        </div>
      </div>
    </div>
  );
}

// Control Panel Component for sink operations
function SinkControlPanel({ sinks, onRefresh }) {
  const [selectedSink, setSelectedSink] = useState('');
  const [isOperating, setIsOperating] = useState(false);

  const handleSinkOperation = async (operation) => {
    if (!selectedSink) {
      alert('Please select a sink');
      return;
    }

    setIsOperating(true);
    try {
      const response = await fetch(`http://localhost:8080/api/sink/${operation}?sinkId=${selectedSink}`, {
        method: 'PUT'
      });
      
      if (!response.ok) {
        throw new Error(`Failed to ${operation} sink`);
      }
      
      alert(`Sink ${operation}d successfully`);
      onRefresh && onRefresh();
    } catch (error) {
      console.error(`Error ${operation}ing sink:`, error);
      alert(`Failed to ${operation} sink`);
    } finally {
      setIsOperating(false);
    }
  };

  return (
    <div className="control-panel">
      <h3>Sink Control Panel</h3>
      
      <div className="control-form">
        <div className="form-group">
          <label>Select Sink:</label>
          <select 
            value={selectedSink} 
            onChange={(e) => setSelectedSink(e.target.value)}
          >
            <option value="">Choose a sink...</option>
            {sinks.map(sinkId => (
              <option key={sinkId} value={sinkId}>
                Sink ID: {sinkId}
              </option>
            ))}
          </select>
        </div>

        <div className="button-group">
          <button 
            onClick={() => handleSinkOperation('enable')} 
            disabled={isOperating || !selectedSink}
            className="btn btn-success"
          >
            {isOperating ? 'Operating...' : 'Enable'}
          </button>
          
          <button 
            onClick={() => handleSinkOperation('disable')} 
            disabled={isOperating || !selectedSink}
            className="btn btn-secondary"
          >
            {isOperating ? 'Operating...' : 'Disable'}
          </button>
        </div>
      </div>
    </div>
  );
}

// Enhanced App Component with additional functionality
function EnhancedFRCVManager() {
  const [activeTab, setActiveTab] = useState('sinks');
  const [refreshTrigger, setRefreshTrigger] = useState(0);
  const [sinks, setSinks] = useState([]);
  const [sources, setSources] = useState([]);

  useEffect(() => {
    loadSinksAndSources();
  }, [refreshTrigger]);

  const loadSinksAndSources = async () => {
    try {
      const [sinksResponse, sourcesResponse] = await Promise.all([
        fetch('http://localhost:8080/api/sink/ids'),
        fetch('http://localhost:8080/api/source/ids')
      ]);
      
      const sinksData = await sinksResponse.json();
      const sourcesData = await sourcesResponse.json();
      
      setSinks(sinksData);
      setSources(sourcesData);
    } catch (error) {
      console.error('Error loading sinks and sources:', error);
    }
  };

  const handleItemCreated = () => {
    setRefreshTrigger(prev => prev + 1);
  };

  return (
    <div className="enhanced-manager">
      <div className="manager-header">
        <h1>FRCV Enhanced Management Interface</h1>
        <p>Create, manage, and control sinks and sources</p>
      </div>

      <nav className="tabs">
        <button 
          className={activeTab === 'sinks' ? 'active' : ''}
          onClick={() => setActiveTab('sinks')}
        >
          Sinks
        </button>
        <button 
          className={activeTab === 'sources' ? 'active' : ''}
          onClick={() => setActiveTab('sources')}
        >
          Sources
        </button>
        <button 
          className={activeTab === 'binding' ? 'active' : ''}
          onClick={() => setActiveTab('binding')}
        >
          Binding
        </button>
        <button 
          className={activeTab === 'control' ? 'active' : ''}
          onClick={() => setActiveTab('control')}
        >
          Control
        </button>
      </nav>

      <div className="content">
        {activeTab === 'binding' && (
          <div className="tab-content">
            <BindingSinkSource 
              sinks={sinks} 
              sources={sources} 
              onRefresh={handleItemCreated} 
            />
          </div>
        )}

        {activeTab === 'control' && (
          <div className="tab-content">
            <SinkControlPanel 
              sinks={sinks} 
              onRefresh={handleItemCreated} 
            />
          </div>
        )}
      </div>
    </div>
  );
}

export { BindingSinkSource, SinkControlPanel, EnhancedFRCVManager };