#[[
  Single source of truth for LumenCore's optional-backend feature flags.

  Replaces four things that previously had to be kept in sync by hand (and regularly weren't -
  see docs/history/IMPLEMENTATION_PLAN.md's phase 8 notes and STEREO_IMPLEMENTATION_PLAN.md ss10.2
  for two real incidents this caused): LumenCore.vcxproj's LumenCommonDefines/LumenPlatformDefines
  MSBuild properties, the vcxproj's own swig PreBuildEvent -D list, Server.csproj's separate
  RunSwig target -D list, and install-deps.sh's feature-dependent build steps.

  Two lists come out of this file, and they are DELIBERATELY NOT THE SAME LIST:

  - LUMEN_ENABLED_DEFINES: only the flags that are actually ON for this configure. Feeds the C++
    compiler (target_compile_definitions) - this is what decides what code exists in the built
    .so/.dll.

  - LUMEN_SWIG_DEFINES: always the FULL flag list, regardless of what's enabled. Feeds swig's own
    -D list.

  If SWIG's -D list tracked the enabled set, two presets with different feature sets (e.g. a
  Windows preset with LUMEN_WITH_WEBRTC off, alongside a Linux preset with it on) would generate
  DIFFERENT C# APIs from the same swig.i into the same Server/Interop/ directory - silently, since
  nothing fails, a method just isn't there. Keeping the SWIG-visible surface constant across every
  preset and handling actual unavailability at runtime instead (see Manager::GetEnabledFeatures()
  and the "declaration always exists, body throws if disabled" pattern in Manager.cpp) is the only
  version of this that's safe. See ROADMAP.md Phase A2 for the fuller rationale.
]]

set(_LUMEN_FEATURE_NAMES
    ONNX
    NT4
    WEBRTC
    RECORD
    VULKAN_APRILTAG
    CODEC_STEREO
    RKNN
    RGA
)

set(_LUMEN_FEATURE_ONNX_DESC            "ONNX Runtime object detection backend")
set(_LUMEN_FEATURE_NT4_DESC             "NetworkTables 4 publishing sink")
set(_LUMEN_FEATURE_WEBRTC_DESC          "WebRTC live-view sink")
set(_LUMEN_FEATURE_RECORD_DESC          "RecordSink - segmented MP4 recording with a JSON-Lines telemetry sidecar")
set(_LUMEN_FEATURE_VULKAN_APRILTAG_DESC "Vulkan compute AprilTag backend")
set(_LUMEN_FEATURE_CODEC_STEREO_DESC    "codec-stereo hardware-motion-vector depth backend")
set(_LUMEN_FEATURE_RKNN_DESC            "Rockchip NPU object detection backend (aarch64 only)")
set(_LUMEN_FEATURE_RGA_DESC              "Rockchip RGA hardware BGR->NV12 conversion for WebRTCSink (aarch64 only)")

# Defaults match what LumenCore.vcxproj's FeatureFlags PropertyGroup used to hardcode: everything
# on except RKNN and RGA, both Rockchip-silicon-only (see their aarch64-only guards below) - a
# generic x64/Windows configure has no such hardware to target regardless of what a caller passes.
set(_LUMEN_FEATURE_ONNX_DEFAULT            ON)
set(_LUMEN_FEATURE_NT4_DEFAULT             ON)
set(_LUMEN_FEATURE_WEBRTC_DEFAULT          ON)
set(_LUMEN_FEATURE_RECORD_DEFAULT          ON)
set(_LUMEN_FEATURE_VULKAN_APRILTAG_DEFAULT ON)
set(_LUMEN_FEATURE_CODEC_STEREO_DEFAULT    ON)
set(_LUMEN_FEATURE_RKNN_DEFAULT            OFF)
set(_LUMEN_FEATURE_RGA_DEFAULT              OFF)

set(LUMEN_SWIG_DEFINES "")
set(LUMEN_ENABLED_DEFINES "")
set(LUMEN_ENABLED_FEATURES "")

foreach(_name ${_LUMEN_FEATURE_NAMES})
    option(LUMEN_WITH_${_name} "${_LUMEN_FEATURE_${_name}_DESC}" ${_LUMEN_FEATURE_${_name}_DEFAULT})
    list(APPEND LUMEN_SWIG_DEFINES "LUMEN_WITH_${_name}")
    if(LUMEN_WITH_${_name})
        list(APPEND LUMEN_ENABLED_DEFINES "LUMEN_WITH_${_name}")
        list(APPEND LUMEN_ENABLED_FEATURES "${_name}")
    endif()
endforeach()

# RKNN (RknnDetectionBackend) and RGA (RgaColorConverter) are both Rockchip-silicon-only -
# refuse either anywhere but a genuine aarch64 configure, rather than let it silently do nothing
# on Windows/x64 (ROADMAP.md Phase 2f).
if(LUMEN_WITH_RKNN AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    message(FATAL_ERROR
        "LUMEN_WITH_RKNN=ON but CMAKE_SYSTEM_PROCESSOR is '${CMAKE_SYSTEM_PROCESSOR}' - "
        "RKNN is the Orange Pi's NPU and only exists on aarch64. Turn it off for this configure.")
endif()
if(LUMEN_WITH_RGA AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    message(FATAL_ERROR
        "LUMEN_WITH_RGA=ON but CMAKE_SYSTEM_PROCESSOR is '${CMAKE_SYSTEM_PROCESSOR}' - "
        "RGA is the Orange Pi's 2D accelerator and only exists on aarch64. Turn it off for this configure.")
endif()

message(STATUS "LumenCore enabled features: ${LUMEN_ENABLED_FEATURES}")
