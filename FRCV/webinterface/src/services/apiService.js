import React, { useState, useEffect } from 'react';

// API service functions
const API_BASE_URL = 'http://localhost:8080/api'; // Adjust based on your server configuration

const apiService = {
  // Sink APIs
  async getSinks() {
    const response = await fetch(`${API_BASE_URL}/sink/ids`);
    return response.json();
  },
  
  async getSinkById(sinkId) {
    const response = await fetch(`${API_BASE_URL}/sink/getSinkById?sinkId=${sinkId}`);
    return response.json();
  },
  
  async createSink(name, type) {
    const response = await fetch(`${API_BASE_URL}/sink/add?name=${encodeURIComponent(name)}&type=${encodeURIComponent(type)}`, {
      method: 'POST'
    });
    return response.json();
  },
  
  async deleteSink(sinkId) {
    const response = await fetch(`${API_BASE_URL}/sink/delete?sinkId=${sinkId}`, {
      method: 'DELETE'
    });
    if (!response.ok) {
      throw new Error('Failed to delete sink');
    }
  },

  async enableSink(sinkId) {
    const response = await fetch(`${API_BASE_URL}/sink/enable?sinkId=${sinkId}`, {
      method: 'PUT'
    });
    if (!response.ok) {
      throw new Error('Failed to enable sink');
    }
  },

  async disableSink(sinkId) {
    const response = await fetch(`${API_BASE_URL}/sink/disable?sinkId=${sinkId}`, {
      method: 'PUT'
    });
    if (!response.ok) {
      throw new Error('Failed to disable sink');
    }
  },

  async bindSinkToSource(sourceId, sinkId) {
    const response = await fetch(`${API_BASE_URL}/sink/bind?sourceId=${sourceId}&sinkId=${sinkId}`, {
      method: 'PUT'
    });
    if (!response.ok) {
      throw new Error('Failed to bind sink to source');
    }
  },

  async unbindSinkFromSource(sinkId, sourceId) {
    const response = await fetch(`${API_BASE_URL}/sink/unbind?sinkId=${sinkId}&sourceId=${sourceId}`, {
      method: 'PUT'
    });
    if (!response.ok) {
      throw new Error('Failed to unbind sink from source');
    }
  },
  
  // Source APIs
  async getSources() {
    const response = await fetch(`${API_BASE_URL}/source/ids`);
    return response.json();
  },
  
  async getSourceById(sourceId) {
    const response = await fetch(`${API_BASE_URL}/source?sourceId=${sourceId}`);
    return response.json();
  },
  
  async getCameraHardwareInfos() {
    const response = await fetch(`${API_BASE_URL}/source/camera/hardwareInfos`);
    return response.json();
  },
  
  async createCameraSource(hardwareInfo) {
    const response = await fetch(`${API_BASE_URL}/source/createCameraSource`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(hardwareInfo)
    });
    return response.json();
  },
  
  async createVideoFileSource(file, fps) {
    const formData = new FormData();
    formData.append('file', file);
    
    const response = await fetch(`${API_BASE_URL}/source/createVideoFileSource?fps=${fps}`, {
      method: 'POST',
      body: formData
    });
    return response.json();
  },
  
  async createImageFileSource(file) {
    const formData = new FormData();
    formData.append('file', file);
    
    const response = await fetch(`${API_BASE_URL}/source/createImageFileSource`, {
      method: 'POST',
      body: formData
    });
    return response.json();
  },
  
  async deleteSource(sourceId) {
    const response = await fetch(`${API_BASE_URL}/source/delete?sourceId=${sourceId}`, {
      method: 'DELETE'
    });
    if (!response.ok) {
      throw new Error('Failed to delete source');
    }
  }
};

export default apiService;