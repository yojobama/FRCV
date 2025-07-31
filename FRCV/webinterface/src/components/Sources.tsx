import React, { useEffect, useState } from 'react';
import { Card, CardContent, Typography, CircularProgress, Box } from '@mui/material';
import axios from 'axios';

interface Source {
  id: number;
  name: string;
}

const Sources: React.FC = () => {
  const [sources, setSources] = useState<Source[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    axios.get('/source/ids')
      .then(response => {
        setSources(response.data.map((id: number) => ({ id, name: `Source ${id}` })));
        setLoading(false);
      })
      .catch(error => {
        console.error('Error fetching sources:', error);
        setLoading(false);
      });
  }, []);

  if (loading) {
    return <CircularProgress />;
  }

  return (
    <Box>
      <Typography variant="h5" gutterBottom>
        Sources
      </Typography>
      {sources.map(source => (
        <Card key={source.id} sx={{ marginBottom: 2 }}>
          <CardContent>
            <Typography variant="h6">{source.name}</Typography>
            <Typography>ID: {source.id}</Typography>
          </CardContent>
        </Card>
      ))}
    </Box>
  );
};

export default Sources;