import React from 'react';
import { Grid, Typography, Box } from '@mui/material';
import Sources from './Sources';
import Sinks from './Sinks';

const Dashboard: React.FC = () => {
  return (
    <Box sx={{ padding: 2 }}>
      <Typography variant="h4" gutterBottom>
        Dashboard
      </Typography>
      <Grid container spacing={2}>
        <Grid item xs={12} md={6}>
          <Sources />
        </Grid>
        <Grid item xs={12} md={6}>
          <Sinks />
        </Grid>
      </Grid>
    </Box>
  );
};

export default Dashboard;