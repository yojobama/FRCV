import React, { useEffect } from 'react';
import { CheckCircle, AlertCircle, Circle, XCircle, X } from 'lucide-react';
import { ToastProps } from '../types';

export const Toast: React.FC<ToastProps> = ({ message, type, onClose }) => {
  useEffect(() => {
    const timer = setTimeout(onClose, 5000);
    return () => clearTimeout(timer);
  }, [onClose]);

  const getToastColor = () => {
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
    <div className={`fixed top-4 right-4 ${getToastColor()} text-white px-4 py-2 rounded-lg shadow-lg z-50 animate-slide-up`}>
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
};