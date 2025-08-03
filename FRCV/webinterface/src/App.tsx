import React, { useState } from 'react';
import { ThemeProvider, createTheme } from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';
import Dashboard from './components/Dashboard';
import ThemeToggle from './components/ThemeToggle';
import './App.css';

function App() {
  const [darkMode, setDarkMode] = useState(true);

  const theme = createTheme({
    palette: {
      mode: darkMode ? 'dark' : 'light',
    },
  });

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <ThemeToggle darkMode={darkMode} setDarkMode={setDarkMode} />
      <Dashboard />
    </ThemeProvider>
  );
}

export default App;
