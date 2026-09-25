#!/usr/bin/env node
// ROADMAP.md Phase 8/E6: regenerates src/api/generated.ts from the C# server's own
// reflection-generated OpenAPI document (Server/OpenApi/OpenApiGenerator.cs, served at
// /api/openapi.json - nothing consumed that document until this script existed). A manual/CI
// step, not part of the hot dev loop: `npm run dev` never needs a live server just to start, and
// this only runs when someone deliberately wants to pick up a REST surface change.
//
// A Node script, not a .sh/.ps1 - this project's dev machines and CI runners span Windows and
// Linux (see the project's own CMakePresets.json), and Node is already a hard dependency of this
// package on every one of them, unlike either shell.
import { spawn } from 'node:child_process';
import { mkdir, writeFile } from 'node:fs/promises';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import openapiTS, { astToString } from 'openapi-typescript';

const __dirname = dirname(fileURLToPath(import.meta.url));
const REPO_ROOT = resolve(__dirname, '..', '..');
const SERVER_URL = 'http://localhost:5800/api/openapi.json';
const OUTPUT_PATH = resolve(__dirname, '..', 'src', 'api', 'generated.ts');

async function isServerUp() {
	try {
		const response = await fetch(SERVER_URL, { signal: AbortSignal.timeout(1000) });
		return response.ok;
	} catch {
		return false;
	}
}

async function waitForServer(timeoutMs) {
	const deadline = Date.now() + timeoutMs;
	while (Date.now() < deadline) {
		if (await isServerUp()) return true;
		await new Promise(r => setTimeout(r, 500));
	}
	return false;
}

async function main() {
	let serverProcess = null;
	let startedOwnServer = false;

	if (await isServerUp()) {
		console.log(`Using the already-running server at ${SERVER_URL}`);
	} else {
		console.log('No server running - starting one with `dotnet run` (this needs LumenCore already built for this platform, e.g. via `cmake --build --preset <your preset>`)...');
		serverProcess = spawn('dotnet', ['run', '--project', resolve(REPO_ROOT, 'Server', 'Server.csproj')], {
			cwd: REPO_ROOT,
			stdio: 'inherit',
		});
		startedOwnServer = true;

		const up = await waitForServer(60000);
		if (!up) {
			serverProcess.kill();
			throw new Error(`Server never became reachable at ${SERVER_URL} within 60s - check the dotnet output above.`);
		}
	}

	try {
		console.log(`Fetching OpenAPI document from ${SERVER_URL}...`);
		const ast = await openapiTS(new URL(SERVER_URL));
		const output = astToString(ast);

		await mkdir(dirname(OUTPUT_PATH), { recursive: true });
		await writeFile(
			OUTPUT_PATH,
			`// GENERATED FILE - do not edit by hand.\n// Regenerate with: npm run generate-api (webui/scripts/generate-api.mjs)\n// Source: Server/OpenApi/OpenApiGenerator.cs's reflection-generated /api/openapi.json.\n\n${output}`,
		);
		console.log(`Wrote ${OUTPUT_PATH}`);
	} finally {
		if (startedOwnServer && serverProcess) {
			serverProcess.kill();
		}
	}
}

main().catch(err => {
	console.error(err);
	process.exit(1);
});
