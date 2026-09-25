import { defineConfig } from 'vite';
import plugin from '@vitejs/plugin-react';

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [plugin()],
    server: {
        port: 57721,
        proxy: {
            '/api': {
                target: 'http://localhost:5800',
                changeOrigin: true,
                secure: false,
            },
            // ROADMAP.md Phase 8c: the /ws/state push channel useStateSocket.ts connects to -
            // without this, `npm run dev` never gets live graph data at all (window.location.host
            // is the Vite dev port, which only the /api proxy above forwards; ws:true is required
            // for Vite to actually upgrade the connection instead of proxying it as plain HTTP).
            '/ws': {
                target: 'ws://localhost:5800',
                ws: true,
                changeOrigin: true,
            }
        }
    }
})
