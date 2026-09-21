# FRCV

A PhotonVision-style FRC vision coprocessor: AprilTag detection (CPU or Vulkan-accelerated),
YOLOv8/v11 object detection, camera calibration, WebRTC live view, and NetworkTables 4
publishing — targeting an Orange Pi 5 / 5 Plus (RK3588) in production, with any Linux box (WSL2
included) usable for development.

See `IMPLEMENTATION_PLAN.md` for the full design rationale, phase-by-phase status, and open
risks/decisions. This file is the quick-start.

## Architecture

- **`FRCVLib/`** — the C++ core: sources (camera/video/image), sinks (AprilTag, object
  detection, calibration, NetworkTables, WebRTC), built as `libFRCVLib.so`.
- **`Server/`** — a .NET 10 EmbedIO web server, gluing the C++ library (via SWIG bindings) to
  the WebUI.
- **`reactproject1/`** — the React/Vite WebUI.
- **`third_party/vkapriltag/`** — git submodule; Vulkan-compute AprilTag detection
  (`git submodule update --init` after cloning).
- **`third_party/codec-stereo/`** — git submodule; stereo depth via hardware video-encoder
  motion vectors (see `STEREO_IMPLEMENTATION_PLAN.md`).

## Building

**No CMake, no Linux-native build.** Visual Studio drives everything: `FRCVLib.vcxproj` builds
inside WSL2 (`Debug|x64` / `Release|x64`, local dev) or remotely on the Orange Pi over SSH via
the Remote_GCC toolset (`Debug|ARM64` / `Release|ARM64`, the actual target). `Server.csproj`
consumes the built `.so` as a prebuilt artifact — it does not build FRCVLib itself, so
`dotnet publish` works even from a machine with no C++ toolchain, as long as the `.so` exists.

1. Copy `FRCVLib/Local.props.example` to `FRCVLib/Local.props` and fill in your WSL distro name
   and/or the Orange Pi's SSH details (gitignored — never commit a password there; Visual
   Studio's Connection Manager handles credentials separately).
2. Run the dependency script **on the machine that will compile**:
   - Inside WSL, for the `x64` configurations: `./scripts/install-deps.ps1 -Target wsl`
   - On the Orange Pi, for the `ARM64` configurations: `./scripts/install-deps.ps1 -Target pi`
     (or run `scripts/install-deps.sh` directly over SSH)
   - `install-deps.sh` builds OpenCV 5.0 and a few other dependencies from source; expect the
     first run to take an hour or more, especially on the Pi's ARM cores.
3. Open `FRCV.sln` in Visual Studio, pick a configuration, build.
4. `git submodule update --init` before building with `FRCV_WITH_VULKAN_APRILTAG` (on by
   default) — `third_party/vkapriltag` needs to be checked out.

### Feature flags

Set as MSBuild preprocessor defines in `FRCVLib.vcxproj` (`FrcvCommonDefines` /
`FrcvPlatformDefines`), not build options — there's no CMake to hold them:

| Flag | Default | Notes |
|---|---|---|
| `FRCV_WITH_ONNX` | on, all platforms | Stock ONNX Runtime, CPU execution provider only |
| `FRCV_WITH_NT4` | on, all platforms | Needs `ntcore`/`wpiutil`/`wpinet` (install-deps.sh `--with-nt4`) |
| `FRCV_WITH_WEBRTC` | on, all platforms | Needs `libdatachannel` (install-deps.sh `--with-webrtc`) |
| `FRCV_WITH_VULKAN_APRILTAG` | on, all platforms | Needs the `vkapriltag` submodule + a Vulkan compute device; falls back to CPU automatically if none is found |
| `FRCV_WITH_RKNN` | ARM64 only | The Orange Pi's NPU; not yet implemented for object detection (throws) |
| `FRCV_WITH_CODEC_STEREO` | on, all platforms | Needs the `codec-stereo` submodule (install-deps.sh builds it by default, unconditionally); `STEREO_BACKEND_SGBM` remains available without this flag |

## Deploying to the Orange Pi

`scripts/deploy.ps1` — publishes the server self-contained for `linux-arm64` (no .NET needed on
the Pi), builds the WebUI, copies both plus the Visual-Studio-built `libFRCVLib.so` to
`/opt/frcv`, and installs/restarts the `frcv` systemd unit (`scripts/frcv.service`). Run it from
Windows after building the `ARM64`/`Release` configuration in Visual Studio.

The Pi is reachable at `frcv.local` once `avahi-daemon` is running (installed by
`install-deps.sh`).

## NetworkTables

NT4 publishing runs in C++ (`NetworkTablesSink`, against the real `ntcore`/`wpiutil`) — there is
no official WPILib C# binding, so this deliberately isn't a P/Invoke round-trip through the
server. Create one via `POST /api/networkTablesSink/createForTeam` (or `createForServer` for
bench testing against a local NT4 server), bind it to one or more detector nodes, and it
publishes each into its own subtable under the configured root table (default `/FRCV`).

## Calibration

`POST /api/cameraCalibrationSink/create` (default 6x9 checkerboard, 25mm squares) or
`createWithBoard` for a custom checkerboard or ChArUco board. Point it at a camera, call
`.../saveDetection` for each snapshot (need at least 4), then `.../run` to compute the result —
calibration only happens when explicitly asked, never implicitly. Results persist to
`calibrations.json`, keyed by camera device path + resolution.

## Object detection

Upload a YOLOv8 or YOLOv11 ONNX export (`ultralytics export format=onnx`) via
`POST /api/model/upload` (multipart: `model`, optional `labels`, plus `variant`/`inputSize`/
thresholds as form fields), then create a sink with `POST /api/objectDetectionSink/create`
referencing the returned model id. RKNN (the actual production path on the Pi's NPU) is not
implemented yet — only the ONNX Runtime CPU backend runs today.

## Live view

`WebRTCSink` streams any single frame-producing node (a raw camera, or a detector's annotated
output) over WebRTC (`libdatachannel` + ffmpeg `libx264`; hardware encoding via `h264_rkmpp` on
the Pi is a follow-up). Signalling is plain REST (`/api/webrtcSink/*`), not a persistent
WebSocket — FRCV uses non-trickle ICE on its side, so one offer/answer/candidate exchange over
ordinary HTTP requests is enough.

## Stereo depth

See `STEREO_IMPLEMENTATION_PLAN.md` for the full design (disparity-window derivation, sign
convention, accuracy expectations, and the risks around camera synchronization worth reading
before buying stereo hardware). Two nodes: `StereoCalibrationSink` and `StereoDepthSink`, both
bound to a left/right camera pair via `PATCH /api/stereoCalibrationSink/{id}/bind` or
`.../stereoDepthSink/{id}/bind` (`leftSourceId`/`rightSourceId`) — the ordinary single-source
`/api/sink/bind` doesn't apply here, since getting left/right backwards silently flips the sign
of every disparity.

Calibrate first: `POST /api/stereoCalibrationSink/create` (default 6x9 checkerboard, 25mm
squares — ChArUco isn't supported for stereo yet), bind both cameras, call
`.../saveDetection` for each pair with the board visible to both eyes (need at least 8),
then `.../run`. Check the returned `epipolarRms`, not `stereoRms` — gate real use at < 0.5px,
since that's what predicts whether the depth node will actually produce dense output. Results
persist to `stereoCalibrations.json`, keyed by both cameras' device paths + resolution.

Then `POST /api/stereoDepthSink/create` with a backend (`STEREO_BACKEND_SGBM` always available;
`STEREO_BACKEND_CODEC_LAVC`/`STEREO_BACKEND_CODEC_RKMPP_HWENC` need `FRCV_WITH_CODEC_STEREO`,
on by default — see the feature-flag table above), a depth range, and the calibration result
from above, then bind the same two cameras. `GET .../stats` returns the last pair's valid
fraction and median depth; the full per-block grid is in `/api/sink/getResult`'s JSON.

Optionally, `POST /api/depthFusionSink/create` fuses a detector's (AprilTag/object detection)
bounding boxes with a `StereoDepthSink`'s depth grid — bind the detector to the depth sink's own
rectified-left frame output (not a raw camera, or its bbox pixel coordinates won't index into
the same grid), then `PATCH .../attachDepthSource` to point it at the depth sink directly (a
plain C++ reference, not a bound source — the full depth grid is deliberately never serialized
through JSON). Each detection comes back with `distanceMeters`/`xMeters`/`yMeters` added.

The WebUI has a "Stereo" tab covering this whole flow — calibration wizard, depth node config,
and fusion — with `epipolarRms` and the 0.5px gate called out explicitly.

## Known gaps

See `IMPLEMENTATION_PLAN.md`'s phase status lines for the authoritative list. Notably: the
WebUI has not been updated for any of the above yet (still the pre-existing single-page app);
RKNN object detection is unimplemented; a stored calibration doesn't yet auto-apply to a
matching camera source on creation; and WebRTC has been verified by compiling/linking against
the real libraries, not against an actual browser handshake (no browser available in this
project's dev environment so far).
