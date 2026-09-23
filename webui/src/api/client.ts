import createClient from 'openapi-fetch';
import type { paths } from './generated';

// ROADMAP.md Phase 8/E6: the typed client ApiService.ts migrates onto incrementally (see its own
// note on why that's incremental, not a big-bang rewrite). Same baseUrl reasoning as
// ApiService's own: relative to wherever this page is served from, not a hardcoded dev-only host.
export const apiClient = createClient<paths>({ baseUrl: `${window.location.origin}/api` });
