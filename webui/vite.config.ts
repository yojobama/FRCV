import { defineConfig } from 'vite';
import plugin from '@vitejs/plugin-react';

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [plugin()],
    server: {
        port: 57721,
        proxy: {
            '/api': {
                target: 'http://localhost:8175',
                changeOrigin: true,
                secure: false,
            }
        }
    }
})
