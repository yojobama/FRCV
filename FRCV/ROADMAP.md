# FRCV — Next-Cycle Roadmap

Companion to `IMPLEMENTATION_PLAN.md` (phases 1–9, mostly landed) and
`STEREO_IMPLEMENTATION_PLAN.md` (phase 10). Those two describe how the current system was
built. This one covers the six things that come next, plus the three cross-cutting decisions
(name, build system, on-board planning) that have to be settled before or alongside them.

Written 2026-09-21, against `feature/stereo-vision` @ `6d403cf`.

---

## 0. The observation that shapes everything below

The six requested items are not six independent work streams. Three of them
— **side-by-side stereo cameras (4)**, **requesting a specific capture resolution (5)** and
**stopping the waste of treating `cv::Mat` as the universal currency (6)** — are all blocked on
the same missing piece: **FRCV has no capture layer.** `CameraFrameSource` is 52 lines that do
`cv::VideoCapture(devicePath, cv::CAP_V4L2)` and `capture >> mat`. It never calls
`set(CAP_PROP_FRAME_WIDTH/HEIGHT/FPS/FOURCC)`, never enumerates what the device can do, and hands
downstream nodes a CPU-side BGR `cv::Mat` that the V4L2 backend produced by decoding MJPEG and
colour-converting on the CPU — before anything has even asked for a frame.

So: build the capture layer once (Phase B), and items 4, 5 and half of 6 fall out of it.

Two more structural facts worth stating up front, because the plan is ordered around them:

- **`UnitTests` compiles exactly one file (`main.cpp`) and every line of it is commented out.**
  The other 30 test files are not in the vcxproj at all, and several reference classes deleted in
  `49a3a2a` (`Frame`, `FramePool`, `PreProcessor`). There is no automated test coverage of any
  kind. Item 3 ("test all sinks") is therefore not an afternoon of manual poking — it is building
  the harness that makes "do the sinks work?" a question a machine can answer, repeatedly.
- **RKNN is not implemented.** `Manager.cpp:801` throws
  `"RKNN object detection backend is not implemented yet - use ONNX"`. Item 3 lists rknn among
  the sinks to test; there is nothing there to test yet. It is also the single largest
  performance win available (6 TOPS of idle NPU vs. YOLO on the A76 cores), so it sits in the
  hardware-offload phase, not the testing one.

---

## Phase A — Foundations

Everything else is cheaper after these. None of them are features; all three are the kind of
work that gets more expensive the longer it waits.

### A1. Settle the name, then rename once *(see Q2)*

**Do this before the vendordep exists, not after.** Once a team has
`implementation 'org.<name>:<name>-java:2026.1.0'` in their `build.gradle` and
`/FRCV/<node>` hardcoded in their dashboard layouts, renaming costs every user a migration.
Before that point it costs one afternoon.

Surface area of a rename: repo name · `FRCVLib`/`Server` project names · the `frcv` Java package
in `RobotSideJava` · NT root table default (`FRCV`) · mDNS hostname (`frcv.local`) · systemd unit
(`frcv.service`) · `/opt/frcv` · `FRCV_WITH_*` preprocessor flags · `libFRCVLib.so` ·
`calibrations.json`/`stereoCalibrations.json` and their sibling config · maven group id · the
`FRCV_BUILD_ROOT` env var in `install-deps.sh`.

Effort: **S** (1 day), and it only ever gets bigger.

### A2. Move FRCVLib to CMake + CMakePresets *(see Q3)*

`FRCVLib.vcxproj` is the only component in the whole tree that is not already CMake — OpenCV,
apriltag, vkapriltag, codec-stereo, libdatachannel and ntcore are all built by `install-deps.sh`
with `cmake -G Ninja`. The vcxproj hardcodes `/usr/local/include;...` include lists and
`../third_party/.../libcodec_stereo.a` paths per configuration, which is exactly where blockers
B1/B2 came from, and encodes feature flags as a semicolon-delimited MSBuild property string
(`FrcvCommonDefines`) that has to be kept in sync by hand with *two* separate `swig` invocations
(`FRCVLib.vcxproj:189` and `Server.csproj:143`).

Target shape:

```
CMakeLists.txt              # project(), option(FRCV_WITH_RKNN ...), add_subdirectory
FRCVLib/CMakeLists.txt      # the .so, plus UseSWIG for the C# bindings
cmake/FindRKNN.cmake  FindRockchipMPP.cmake  FindRGA.cmake
CMakePresets.json           # wsl-x64-{debug,release}, pi-arm64-{debug,release},
                            # linux-native, ci-arm64-container
```

`Server.csproj` and `reactproject1` stay as they are — both are already cross-platform. Keep a
thin `.sln` for those two if the VS muscle memory is worth it.

Deliverables: presets that build identically from VS-on-Windows, from `cmake --build` on any
Linux box, and from CI; `find_package`/`pkg_check_modules` replacing every hardcoded path;
`swig_add_library(... LANGUAGE csharp)` replacing both hand-rolled `swig` calls; a GitHub Actions
workflow producing an arm64 `.deb` via CPack.

Effort: **M** (2–3 days incl. CI). Keep the vcxproj for one release as a fallback, then delete
it — do not maintain both.

### A3. Make the test project real

Prerequisite for item 3, and the thing that keeps items 4–6 from regressing the pipeline.

1. Delete or port the 30 orphaned test files; put what survives into the build. Pick a real
   framework (Catch2 or GoogleTest via CMake `FetchContent` — trivial once A2 lands).
2. **Golden-data tests, no hardware required.** Check in a small corpus:
   - AprilTag frames at known poses (tag36h11, a few distances/angles) with expected ids,
     corners and pose translations within tolerance. Runs both `CpuApriltagBackend` and
     `VkApriltagBackend` against the *same* corpus and asserts they agree — that is the
     regression test for the Vulkan path that does not exist today.
   - A YOLO ONNX model + images with expected boxes, for `OnnxDetectionBackend` and (later)
     `RknnDetectionBackend`, asserted to agree with each other.
   - A synthetic rectified stereo pair with a known disparity field, for `SgbmStereoBackend`
     and `CodecStereoBackend` — this is what catches a sign flip without needing a stereo rig.
   - `ImageFileSource → detector → NetworkTablesSink` end to end against a locally started NT4
     server, asserting the published topic *values*, not just that a connection happened.
3. **A hardware-in-the-loop suite** (`ctest -L hitl`) that runs on the Pi over SSH from
   `deploy.ps1`, and is the only place anything needs a real camera / NPU / VPU.
4. Wire both into CI: golden tests on every push (x64 Linux runner), HITL nightly against the
   bench Pi.

Effort: **L** (4–6 days), and it is the highest-leverage item in this document.

---

## Phase B — The capture layer *(items 4, 5, and the foundation of 6)*

### B1. `V4l2Capture` — replace `cv::VideoCapture`

A direct V4L2 mmap capture class. Not a rewrite for its own sake: `cv::VideoCapture`'s V4L2
backend cannot express "give me 800×600 NV12 at 60 fps with one buffer of latency", and cannot
hand you a DMA-BUF fd, which B2/C2/C3 all need.

- `VIDIOC_ENUM_FMT` / `VIDIOC_ENUM_FRAMESIZES` / `VIDIOC_ENUM_FRAMEINTERVALS` → a real capability
  list per device. (`CameraSource.h` already includes `<linux/videodev2.h>`; the ioctls are
  right there.)
- `VIDIOC_S_FMT` with explicit fourcc + width/height, `VIDIOC_S_PARM` for the frame interval,
  and a **verify-after-set** step: V4L2 silently substitutes the nearest supported mode, so the
  node must report what it actually got and log loudly when it differs from what was asked.
- `V4L2_MEMORY_MMAP` with a small queue (3–4 buffers) to bound latency; export DMA-BUF fds for
  B2.
- Keep `cv::VideoCapture` behind the same interface as a fallback for dev boxes and for
  `VideoFileSource` / `ImageFileSource`, which have no reason to change.

### B2. `Frame` — stop using `cv::Mat` as the universal currency *(item 6, the architectural part)*

Introduce a refcounted `Frame`: `{ format (NV12|YUYV|MJPEG|GRAY8|BGR24|RGB24), width, height,
stride[], plane pointers, optional dmabuf fd, optional cv::Mat view }`. `cv::Mat` becomes *a view
over a Frame*, not the storage. `SourceResult::frame` becomes `std::optional<Frame>`.

Why this matters concretely, in code that exists today:

| Waste today | What `Frame` enables |
|---|---|
| `CameraFrameSource` captures MJPEG, OpenCV decodes it to BGR on the CPU, every frame | keep it compressed until someone needs pixels; decode on the VPU (C2) |
| `ApriltagDetector::Process` does `cvtColor(BGR2GRAY)` every frame | capture NV12/YUYV and the Y plane *is* the grayscale image — zero work, zero copy |
| `ApriltagDetector` and `ObjectDetectionSink` both `clone()` the frame and draw overlays unconditionally | annotate only when a WebRTC/Record sink is actually bound downstream (C4) |
| `WebRTCSink` colour-converts to YUV then encodes with `libx264` | hand the NV12 DMA-BUF straight to `h264_rkmpp` (C2) |
| YOLO preprocessing letterboxes/resizes with `cv::resize` on the CPU | RGA does it in hardware (C3); RKNN also takes NV12 input directly |

Note this is a *return* to something the project deleted (`Frame`/`FramePool`, blocker B10) —
but with a defined purpose this time: format and ownership, not object pooling.

### B3. Resolution / format control, end to end *(item 5)*

- `Manager`: `vector<CameraMode> GetCameraModes(int sourceId)` and
  `CreateCameraSource(info, width, height, fps, fourcc)`, plus `SetCameraMode` on a live source
  (stop → reconfigure → start).
- REST: `GET /api/cameraSource/{id}/modes`, `PATCH /api/cameraSource/{id}/mode`.
- UI: a mode picker showing the *actual* enumerated list, not a free-text box, with the
  resolution/fps/format tradeoff visible (USB 2.0 bandwidth, MJPEG-vs-raw).
- **Invalidation:** calibrations are keyed by device path + resolution (already true for both
  `calibrations.json` and `stereoCalibrations.json`). Changing a camera's mode must mark any
  attached calibration as not-applicable, visibly, in the UI — not silently keep using
  intrinsics computed at a different resolution. This is a correctness bug waiting to happen the
  moment item 5 ships.
- Also expose queue depth and exposure / gain / auto-exposure — a fixed short exposure is what
  actually makes AprilTags work on a moving robot, and the UI has no way to set it today.

### B4. Side-by-side stereo cameras *(item 4)*

A camera that emits both eyes in one frame is, counter-intuitively, **better** than two
independent cameras: both halves share one exposure and one capture timestamp, which deletes the
entire class of problem `STEREO_IMPLEMENTATION_PLAN.md` warns about (`maxSkewUs`, free-running
sensors drifting apart). Treat it as the preferred configuration, not a special case.

Do both of the first two:

1. **`stereoLayout` on the stereo nodes (primary path).** `StereoCalibrator` and
   `StereoDepthNode` gain a layout property: `SEPARATE` (today's two-source binding) |
   `SIDE_BY_SIDE` | `TOP_BOTTOM`. With a split layout they bind *one* source and split it
   internally with two `cv::Mat(frame, roi)` **views** — no copy at all, since `cv::remap` reads
   straight from them. Skew is structurally zero, the `maxSkewUs` gate is bypassed, and the
   left/right-swap footgun collapses to one `swapEyes` boolean rather than a binding order.
2. **A generic `RoiSource` node (secondary).** Binds one source, emits a cropped view. Needed
   whenever you want the two eyes as independent nodes — previewing just the left eye, running a
   detector on one half, calibrating each eye's intrinsics separately. Must propagate the
   upstream `captureTimeUs` through the three-argument `SourceResult` constructor so downstream
   pairing stays exact.
3. Per-eye intrinsic calibration still needs doing; the vendor's published baseline is a sanity
   check and an initial guess for `stereoCalibrate`, **not** a substitute for calibrating. Gate
   on `epipolarRms < 0.5px` as the existing plan already says.

UI: the stereo wizard must ask "one camera or two?" as its first question and branch, rather
than presenting a left/right binding pair that makes no sense for a single-device camera.

Phase B effort: **L** (8–12 days total). B1+B2 are the bulk; B3 and B4 are small once they land.

---

## Phase C — Actually using the hardware *(item 6)*

Ordered by watts-saved-per-day-of-work. **Measure before and after each one** — B2 should land
with per-node timing exposed (`captureTimeUs` / `producedTimeUs` already exist; nothing surfaces
them yet).

### C1. RKNN object detection backend — the big one

Not "testing rknn"; writing it. `RknnDetectionBackend : IDetectionBackend`, mirroring
`OnnxDetectionBackend`, reusing `YoloPostProcess` (which was deliberately factored out for
exactly this). Expected: YOLOv8n 640×640 goes from roughly 100 ms on the A76 cluster to
10–20 ms on one NPU core, and stops competing with everything else for CPU.

- ONNX → RKNN conversion is an offline step (`rknn-toolkit2`, x86 host, Python). Ship a
  documented converter script and accept `.rknn` uploads in `ModelController` alongside `.onnx`,
  with the manifest recording which is which.
- RKNN takes **NV12 input natively** — with B2 in place the camera frame reaches the NPU with no
  CPU colour conversion at all.
- Three NPU cores: `RKNN_NPU_CORE_AUTO` to start, per-core pinning if you end up running two
  detectors.
- Risk already logged (`IMPLEMENTATION_PLAN.md` risk 2): driver `v0.9.7` vs `librknnrt 2.3.2`.
  The first real `rknn_init()` proves or disproves it.

### C2. VPU: hardware encode and decode

- `h264_rkmpp` instead of `libx264` in `WebRTCSink` (already flagged as a follow-up; the encoder
  is already selected by name at `WebRTCSink.cpp:121`, so this is mostly config plus an
  NV12/DMA-BUF path).
- MPP hardware **JPEG decode** for MJPEG cameras — at 1080p30 that is a meaningful chunk of a
  core given back.
- `CS_ENABLE_RKMPP_HWENC` is already built by `install-deps.sh` when `rockchip_mpp` is present;
  verify `STEREO_BACKEND_CODEC_RKMPP_HWENC` actually runs on the board (part of Phase D).

### C3. RGA for 2D work

Resize, crop, colour-space conversion and YOLO letterboxing on the Rockchip 2D engine instead of
`cv::resize` / `cv::cvtColor`. Cheap to add once `Frame` carries DMA-BUF fds.

### C4. Pipeline hygiene — small, free wins

- **Annotate on demand.** `ApriltagDetector::Process` and `ObjectDetectionSink::Process` both
  `clone()` and draw every frame unconditionally. Make it conditional on a downstream frame
  consumer existing. On a headless competition robot this is pure waste today.
- **Grayscale from the Y plane** instead of `cvtColor(BGR2GRAY)`.
- **Fix the frame-count race.** `ISource::m_FrameCount` is a plain `uint64_t` written under
  `m_ResultLock` but read lock-free by `ISink::ProcessingThreadLoop` via `GetCurrentFrameCount()`
  — a data race. And `ISink` stores the last-seen count as `int` (`m_Sources` is
  `vector<pair<shared_ptr<ISource>, int>>`) while comparing against a `uint64_t`: a
  signed/unsigned comparison that breaks outright past 2^31 frames. Make the counter
  `std::atomic<uint64_t>` and the stored copy `uint64_t`.
- **`Process(std::vector<SourceResult> sources)` takes its vector by value** on every wake-up, in
  every sink. Should be `const&`.
- **Thread placement.** RK3588 is 4×A76 (cores 4–7) + 4×A55 (cores 0–3). Every node currently
  gets a bare `pthread_create` with default affinity, so the scheduler is free to land the
  AprilTag detector on a 1.8 GHz A55. Add a node "weight" (heavy/light) and set affinity
  accordingly with `pthread_setaffinity_np`.
- Revisit the OpenCV build flags in `install-deps.sh` — it currently builds with near-defaults.

### C5. A benchmark you can quote

`frcv-bench`: fixed input corpus, per-stage timing, published as a table in the README per
configuration. This is also the marketing material — "we do X fps where PhotonVision does Y" is
the argument that gets teams to switch, and you cannot make it without numbers.

Phase C effort: **L** (10–14 days). C1 alone is 3–5.

---

## Phase D — Verify every sink, for real *(item 3)*

Status going in, so the work is honest about what "test" means for each:

| Sink / backend | State today | What item 3 actually means |
|---|---|---|
| `CpuApriltagBackend` (libapriltag) | verified on real hardware | golden-corpus regression test (A3) |
| `VkApriltagBackend` (Vulkan) | verified on real hardware, tag 585 | agree-with-CPU test on the same corpus; plus the no-Vulkan-device fallback path, which is untested |
| `OnnxDetectionBackend` | implemented, compiles and links; **no verified detection on real hardware** | golden test, then the first real end-to-end run |
| RKNN | **not implemented** (`Manager.cpp:801` throws) | C1 first, then the same tests as ONNX |
| `SgbmStereoBackend` | implemented | synthetic-disparity golden test |
| `CodecStereoBackend` LAVC | implemented | synthetic-disparity golden test |
| `CodecStereoBackend` RKMPP_HWENC | built when `rockchip_mpp` is present | **never run on the board** — needs real hardware |
| `DepthFusionNode` | implemented | needs the new stereo camera (B4) to test meaningfully |
| `NetworkTablesSink` | connects; publishes tags as four parallel arrays and everything else as a `raw` JSON string | E1 replaces the schema anyway; test the new one |
| `WebRTCSink` | verified in a real browser | keep, and add the `h264_rkmpp` path (C2) |
| `RecordSink` | `RecordingSinkController.Create` is `throw new NotImplementedException()` and is not registered in `Program.cs` | decide: implement or delete. It is currently a lie in the API surface. |

Effort: **M** (3–4 days) *given A3*. Without A3 it is an unbounded manual slog that has to be
redone after every change.

---

## Phase E — The vendordep *(item 1)*

### E0. Positioning decision

PhotonVision's API is the *de facto* standard; the closer you are, the cheaper a team's switch.
So: **mirror `photonlib`'s shape deliberately, and ship a compatibility package that makes
migration a one-line import change.** Not a clone — FRCV has things Photon does not (per-target
stereo range, a node graph) — but the 90% path should look familiar enough that a team's existing
`RobotContainer` compiles after an import swap.

### E1. Transport and schema

**NT4 only for robot-side data.** Delete the UDP prototype (`Server/UDPTransmiter.cs`,
`Server/Controllers/UDPController.cs`, `frcv/UDPClient.java`) — two transports is two things to
debug at 11pm in the pit, and NT4 gets you AdvantageScope, Shuffleboard and Elastic for free.
REST stays, for configuration from the WebUI only.

```
/<Root>/.version                (string)   coprocessor version, for the handshake
/<Root>/.status                 (string)   json: uptime, temp, cpu, node count, clock offset
/<Root>/<node>/result           (raw)      canonical versioned packet — what the vendordep reads
/<Root>/<node>/hasTargets       (boolean)  \
/<Root>/<node>/targetYaw        (double)    |  best target, flattened,
/<Root>/<node>/targetPitch      (double)    |  for dashboards and for teams
/<Root>/<node>/targetArea       (double)    |  who never install the vendordep
/<Root>/<node>/targetPose       (struct:Transform3d)
/<Root>/<node>/latencyMs        (double)   /
/<Root>/<node>/fps              (double)
/<Root>/<node>/heartbeat        (int)
/<Root>/<node>/cameraIntrinsics (double[9])
/<Root>/<node>/cameraDistortion (double[8])
/<Root>/<node>/config/pipelineIndex  (int, robot-writable)
/<Root>/<node>/config/driverMode     (boolean, robot-writable)
```

Serialization: a **hand-rolled versioned byte packet** on the `raw` topic — the approach
`photonlib` settled on, for the right reason: results are variable-length, and WPILib's `Struct`
requires a fixed size. Zero new dependencies on either side. The first two bytes are a schema
version; the vendordep refuses to decode a version it does not know and says so loudly. Publish
the fixed-size fields as real struct/number topics *in addition*, so AdvantageScope can graph
them without understanding the packet.

**Time synchronisation is the part that must not be hand-waved.** Pose estimation accuracy rests
entirely on it:

- The coprocessor timestamps captures with `nt::Now()` (offset-corrected against the NT4 server
  once connected), not raw wall clock, and puts that in the packet.
- The vendordep converts to the RIO's FPGA timebase and exposes
  `FrcvPipelineResult.getTimestampSeconds()` in exactly the units
  `SwerveDrivePoseEstimator.addVisionMeasurement()` wants.
- Expose the measured offset and a round-trip estimate in `.status`, so a bad clock is visible
  rather than silently wrong.

### E2. Java API

```java
package org.<name>.lib;

FrcvCamera cam = new FrcvCamera("front");
List<FrcvPipelineResult> unread = cam.getAllUnreadResults();   // the 2025+ Photon idiom
boolean ok = cam.isConnected();

FrcvPipelineResult r = unread.get(unread.size() - 1);
r.hasTargets();  r.getTargets();  r.getBestTarget();
r.getTimestampSeconds();          // FPGA timebase
r.getMultiTagResult();            // Optional<MultiTargetPNPResult>

FrcvTrackedTarget t = r.getBestTarget();
t.getYaw(); t.getPitch(); t.getArea(); t.getSkew();
t.getFiducialId();  t.getDetectedObjectClassId();  t.getDetectedObjectConfidence();
t.getBestCameraToTarget();        // Transform3d
t.getAlternateCameraToTarget();  t.getPoseAmbiguity();
t.getDetectedCorners();
// FRCV-only, and the reason a team would choose it:
t.getStereoDistanceMeters();      // OptionalDouble, from DepthFusionNode
t.getStereoTranslation();         // Optional<Translation3d>

FrcvPoseEstimator est = new FrcvPoseEstimator(
    fieldLayout, PoseStrategy.MULTI_TAG_PNP_ON_COPROCESSOR, robotToCamera);
Optional<EstimatedRobotPose> pose = est.update(r);

FrcvUtils.calculateDistanceToTargetMeters(...);   // same signatures as PhotonUtils
FrcvUtils.estimateCameraToTargetTranslation(...);
FrcvUtils.getYawToPose(...);  FrcvUtils.getDistanceToPose(...);

cam.setPipelineIndex(1);  cam.setDriverMode(true);  cam.takeInputSnapshot();
cam.getCameraMatrix();    // Optional<Matrix<N3,N3>>
```

Plus `org.<name>.photoncompat`, exposing `PhotonCamera`, `PhotonPipelineResult`,
`PhotonTrackedTarget`, `PhotonPoseEstimator` and `PhotonUtils` with identical member signatures
as thin wrappers. A migrating team changes the import line and nothing else. (Deliberately *not*
the literal `org.photonvision` package: it would collide if both vendordeps are installed, and
squatting someone else's namespace is not okay.)

**Version handshake:** on construction, compare `.version` against the library's own and print a
loud, unmissable DriverStation warning on mismatch. The single most common support question in
FRC vision is "I updated one side and not the other."

### E3. What the vendordep forces FRCV to grow

- **Rotation in the AprilTag result.** `ApriltagDetector.cpp` publishes only `pose.t`
  (translation); the rotation lines are commented out. A `Transform3d` needs `pose.R`, and
  `getPoseAmbiguity()` needs the second solution and both errors from
  `estimate_tag_pose_orthogonal_iteration`. **Without this there is no pose estimation, and
  therefore no reason for a team to switch.** Treat it as a blocker on E2, not a nice-to-have.
- **Multi-tag PnP on the coprocessor.** `cv::solvePnP` over all visible tags against the field
  layout in one shot — materially more accurate than averaging single-tag poses, and it is what
  `MULTI_TAG_PNP_ON_COPROCESSOR` means. Needs the season's `AprilTagFieldLayout` JSON on the Pi.
- **Pipeline profiles.** `setPipelineIndex` has no meaning in FRCV today — there is a node graph,
  not a list of pipelines. Add named **profiles**: a saved graph configuration, switchable at
  runtime, indexed for NT compatibility. Teams genuinely need this ("tag mode" in auto, "game
  piece mode" in teleop), and it maps cleanly onto the profiles UI in Phase F.
- **Driver mode.** Bypass processing, raise exposure, stream raw.
- **Snapshots.** `takeInputSnapshot()` / `takeOutputSnapshot()` writing to disk, downloadable
  from the UI. This is how teams debug a match afterwards, and it costs almost nothing to add.

### E4. Packaging

- Pure-Java artifact, no JNI — everything arrives over NT, and the maths is wpimath's.
- Maven repo on GitHub Pages: `https://<org>.github.io/<repo>/repo`.
- `<Name>.json` vendordep manifest with `frcYear`, `uuid`, `mavenUrls`, `jsonUrl`,
  `javaDependencies`; served from the coprocessor itself too — PhotonVision does this and teams
  love it (grab the vendordep from the device you are already looking at).
- C++ vendordep second (`cppDependencies`, headers plus a static lib, no JNI either).
- Python / RobotPy third.
- Simulation (a `VisionSystemSim` equivalent) fourth — design `FrcvCamera` to take an injected
  `NetworkTableInstance` now, so it can slot in later without an API break.

Phase E effort: **L** (10–15 days for Java, schema and profiles; C++ and sim on top).

---

## Phase F — The new UI *(item 2)*

The current UI is four tabs (Dashboard / Sources / Sinks / Stereo) over a 1233-line `App.tsx`,
with a bind-picker dropdown standing in for the pipeline structure. It was right for a
source→sink world; it actively hides the thing that is now the product. `StereoPage.tsx` already
shows the strain — 415 lines of wizard bolted on beside the tab model, because the tab model had
nowhere to put it.

### F0. Plumbing first

1. **`/ws/state` WebSocket** broadcasting `GetPipelineState()` plus per-node results, replacing
   the polling in `useAppData.ts`. A node editor with 5-second polling is unusable.
2. **Generate the client from OpenAPI.** `ApiService.ts` is 583 hand-written lines whose own
   comments complain about drift from the server. Emit an OpenAPI document from the controllers
   and generate `ApiService.ts` + `types/index.ts` from it.
3. **A machine-readable node-capability descriptor** served by the backend: per node type,
   `{ produces: [frame|json], accepts: [frame|json], maxSources, roles: [left,right],
   params: [...] }`. The UI then renders parameter forms automatically and *rejects invalid
   connections structurally* — which kills the entire class of bug the stereo docs currently warn
   about in prose ("bind the detector to the depth node's rectified-left output, not a raw
   camera").

### F1. The graph is the UI

- **Canvas centre-stage** (`@xyflow/react`). Nodes coloured by class: sources, detectors,
  transforms, terminals. Edges are bindings, labelled with their role where roles exist
  (`left` / `right`, `depth`).
- **Terminal sinks are not boxes.** Keep the decision already made in Phase 8: WebRTC and NT
  publishing are *toggles on a node's output*, shown as small badges on the node, not separate
  nodes the user has to wire.
- **Right-hand inspector** for the selected node: parameters with live apply, a live WebRTC
  preview of *that node's* output, its latest result JSON, and its FPS / latency / backend name.
  Everything needed to tune a detector without leaving the canvas.
- **Left rail**: Profiles · Cameras · Models · Calibrations · Logs · Settings.
- **Bottom strip**: thumbnails of every previewed node, plus a global bar (NT connected, CPU,
  temperature, aggregate FPS).

### F2. Wizards for the multi-step flows

Calibration and stereo calibration are inherently step-by-step and do not belong in a modal.
Full-screen takeovers, with:

- a **live coverage heatmap** showing where board detections have landed in the frame, so the
  operator knows to move toward the corners — the single biggest usability gap in every existing
  FRC calibration tool;
- the snapshot count and the acceptance gate (≥ 4 mono, ≥ 8 stereo) shown as progress;
- `epipolarRms` with the 0.5 px gate as a pass/fail light, not a number in a table;
- a first step that asks **one camera (side-by-side) or two**, per B4.

### F3. Profiles and match mode

- **Profiles dropdown** at the top: switching swaps the entire graph. The same concept the
  vendordep exposes as `pipelineIndex`.
- **Match view**: one read-only page showing every camera's FPS, latency, NT state, temperature
  and a preview. No editing, nothing destructive, readable across a pit. This is what is on
  screen during a competition, and no current FRC vision UI does it well.

### F4. Constraints worth writing down

- **Fully offline.** No CDN fonts, no external assets, no telemetry. Venue WiFi is hostile and
  often absent; the page is served from the Pi.
- **Light.** It runs on the robot's coprocessor while that coprocessor is doing real work.
- Dark by default — pits are dark, and the UI ends up on a laptop next to a bright field.
- Keep React + Vite + Tailwind; add `@xyflow/react` and a small store (`zustand`). Split
  `App.tsx` into routed pages — the router is a prerequisite for everything above.

Phase F effort: **L** (12–18 days). F0 is a third of it and unblocks the rest.

---

## Phase G — Optional: on-board trajectory planning

Only after Phase C. The short version: it fits in the compute budget once the vision work is off
the CPU; "drive to the game piece you detected and ranged" should ship long before "avoid other
robots"; and the Pi must publish *paths and obstacle sets*, never wheel commands. See the Q1
discussion accompanying this roadmap for the full analysis.

---

## Suggested order

```
A1 name ──┬─> A2 cmake ──> A3 tests ──┬─> B1/B2 capture+Frame ──┬─> B3 resolution
          │                           │                         ├─> B4 side-by-side
          │                           │                         └─> C1 rknn ─┬─> C2 vpu
          │                           │                                      ├─> C3 rga
          │                           │                                      └─> C4/C5
          │                           └─> D verify sinks (per backend, as each lands)
          └─> E1 schema ──> E3 rotation+multitag+profiles ──> E2 java lib ──> E4 publish
                                    │
                                    └─> F0 ws+openapi+capabilities ──> F1 graph ──> F2/F3
```

Practical sequencing: **A1 → A2 → A3 → E3's rotation fix → B → C1 → E → F**, with D folded in
continuously. Two things deserve to jump the queue regardless of everything else:

1. **A1 (the name)**, because it is the only item that gets permanently more expensive.
2. **AprilTag rotation and pose ambiguity** (part of E3). It is a handful of lines in
   `ApriltagDetector.cpp` against an API that is already being called, and without it the
   vendordep cannot do the one thing teams actually want a vision coprocessor for.
