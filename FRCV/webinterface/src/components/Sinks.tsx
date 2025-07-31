import React, { useEffect, useState } from 'react';
import { Card, CardContent, Typography, CircularProgress, Box } from '@mui/material';
import axios from 'axios';

interface Sink {
  id: number;
  name: string;
}

const Sinks: React.FC = () => {
  const [sinks, setSinks] = useState<Sink[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    axios.get('/sink/ids')
      .then(response => {
        setSinks(response.data.map((id: number) => ({ id, name: `Sink ${id}` })));
        setLoading(false);
      })
      .catch(error => {
        console.error('Error fetching sinks:', error);
        setLoading(false);
      });
  }, []);

  if (loading) {
    return <CircularProgress />;
  }

  return (
    <Box>
      <Typography variant="h5" gutterBottom>
        Sinks
      </Typography>
      {sinks.map(sink => (
        <Card key={sink.id} sx={{ marginBottom: 2 }}>
          <CardContent>
            <Typography variant="h6">{sink.name}</Typography>
            <Typography>ID: {sink.id}</Typography>
          </CardContent>
        </Card>
      ))}
    </Box>
  );
};

export default Sinks;