#[[
  The core C++ library, plus its SWIG-generated C# bindings. Everything in this file replaces
  LumenCore.vcxproj (the WSL2/Remote_GCC MSBuild toolset) - see ROADMAP.md Phase A2 for why: that
  project's ApplicationType=Linux is a project-level global, so a Windows/MSVC configuration
  cannot be added to it at all, which is what actually forces this migration rather than it being
  a nice-to-have.
]]

find_package(SWIG 4.0 REQUIRED COMPONENTS csharp)
include(UseSWIG)

set(LUMEN_CORE_SOURCES
    ApriltagDetection.cpp
    ApriltagDetector.cpp
    CameraCalibrationResult.cpp
    CameraCalibrator.cpp
    CameraSource.cpp
    CpuApriltagBackend.cpp
    ISource.cpp
    ImageFileSource.cpp
    StereoCalibrationResult.cpp
    StereoCalibrator.cpp
    SgbmStereoBackend.cpp
    CodecStereoBackend.cpp
    StereoDepthNode.cpp
    DepthFusionNode.cpp
    ISink.cpp
    Logger.cpp
    Manager.cpp
    NetworkTablesSink.cpp
    ObjectDetection.cpp
    ObjectDetectionSink.cpp
    OnnxDetectionBackend.cpp
    RecordSink.cpp
    SourceResult.cpp
    SystemMonitor.cpp
    VideoFileSource.cpp
    VkApriltagBackend.cpp
    WebRTCSink.cpp
    YoloPostProcess.cpp
)

# --- SWIG source properties -------------------------------------------------------------------
# Generated C# lands directly in Server/Interop/ (gitignored, regenerated every configure/build -
# see .gitignore's swig_wrap.cxx / **/Interop/*.cs rules) so Server.csproj's own glob picks it up
# with no path translation step. The generated C++ wrapper (swig_wrap.cxx) stays inside the build
# tree instead, via OUTFILE_DIR - it has no reason to live next to hand-written sources.
set_property(SOURCE swig.i PROPERTY CPLUSPLUS ON)
# Lets swig.i's own #include chain (Manager.h -> ISink.h -> SourceResult.h ->
# <opencv2/opencv.hpp>, etc.) resolve during SWIG's preprocessing pass by reusing whatever
# target_include_directories the LumenCore target ends up with below - confirmed necessary
# (swig.i's own comments already document this: SWIG follows #include chains fine, but only
# if its own -I list covers them).
set_property(SOURCE swig.i PROPERTY USE_TARGET_INCLUDE_DIRECTORIES ON)
# The single source of truth from cmake/LumenFeatures.cmake - deliberately the FULL flag list
# (LUMEN_SWIG_DEFINES), not just what's enabled for this configure. See that file's own comment
# for why: this is what keeps every preset generating the identical C# API regardless of which
# backends are actually compiled in.
set_property(SOURCE swig.i PROPERTY COMPILE_DEFINITIONS ${LUMEN_SWIG_DEFINES})

swig_add_library(LumenCore
    TYPE SHARED
    LANGUAGE csharp
    OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Server/Interop"
    OUTFILE_DIR "${CMAKE_CURRENT_BINARY_DIR}/swig"
    SOURCES swig.i ${LUMEN_CORE_SOURCES}
)

# swig_wrap.cxx (SWIG's generated wrapper) lands in the BUILD tree (OUTFILE_DIR above), not next
# to the hand-written sources the way the old vcxproj's PreBuildEvent generated it directly into
# LumenCore/ itself - its own #include "Manager.h" (from swig.i's %{ %} block) is a quote-include,
# which searches relative to the INCLUDING file's own directory first. Without this, that search
# fails ("Manager.h: No such file or directory") since swig/ and LumenCore/ are now different
# directories. USE_TARGET_INCLUDE_DIRECTORIES ON (set on swig.i above) is what makes SWIG's own
# preprocessing pass pick this up too, for the same reason.
target_include_directories(LumenCore PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# No PREFIX override: the default "lib" prefix on Linux (libLumenCore.so) plus no prefix on
# Windows (LumenCore.dll) is exactly what makes one DllImport("LumenCore") string in the
# generated C# resolve correctly on both platforms under .NET's default native-library probing.
set_target_properties(LumenCore PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_definitions(LumenCore PRIVATE ${LUMEN_ENABLED_DEFINES})

if(MSVC)
    target_compile_options(LumenCore PRIVATE /W3 /permissive-)
else()
    target_compile_options(LumenCore PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# --- Required dependencies (every configuration needs these) ---------------------------------
if(NOT DEFINED OpenCV_DIR AND EXISTS "/usr/local/lib/cmake/opencv5")
    # This machine's /usr/local carries BOTH an OpenCV 5.0 install and a stale OpenCV 4.15 one
    # (install-deps.sh's own history explains why) with similarly-named config directories
    # (opencv5/ vs opencv4/) - find_package(OpenCV)'s default search can't be trusted to prefer
    # the right one, so point it explicitly rather than relying on directory-glob order. Any
    # preset (Windows in particular, once OpenCV is built there) overrides OpenCV_DIR itself;
    # this is only a default for this project's own known Linux install-deps.sh layout.
    set(OpenCV_DIR "/usr/local/lib/cmake/opencv5" CACHE PATH "OpenCV 5.x config directory")
endif()
find_package(OpenCV 5.0 REQUIRED CONFIG
    COMPONENTS core imgproc imgcodecs videoio dnn objdetect calib stereo geometry
)
find_package(Threads REQUIRED)

target_link_libraries(LumenCore PRIVATE ${OpenCV_LIBS} Threads::Threads)
target_include_directories(LumenCore PRIVATE ${OpenCV_INCLUDE_DIRS})

# SgbmStereoBackend (STEREO_BACKEND_SGBM) is pure OpenCV with no feature flag guarding it at
# all - it is always available, on every platform, regardless of which optional backends below
# are turned on. See ROADMAP.md's note that this is what makes stereo depth work on Windows
# from day one even with LUMEN_WITH_CODEC_STEREO off.

# --- apriltag: exactly one, enforced by the build graph, not by hand discipline ---------------
# install-deps.sh's own long comment documents the old rule at length: the system apriltag and
# vkapriltag's private FetchContent copy produce a shared library with the SAME SONAME
# (libapriltag.so.3) regardless of the 3.3.0-vs-3.4.5 version difference, and only the patched
# 3.4.5 build exports the symbols (quad_decode_index, reconcile_detections) VkApriltagBackend
# needs - so there must be exactly one apriltag in the system, and it must be the patched one.
# That was enforced entirely by convention before (install-deps.sh purging apt's package, a
# human remembering to build the patched version first). CMake can make it structural instead:
#
# when LUMEN_WITH_VULKAN_APRILTAG is ON, vkapriltag's own subdirectory is added FIRST and its
# FetchContent-built (patched) `apriltag` target becomes the ONE apriltag the rest of this file
# links against - including CpuApriltagBackend.cpp, which has no Vulkan dependency of its own
# but has no reason to use a DIFFERENT apriltag build than VkApriltagBackend.cpp when both are
# compiled in. (FETCHCONTENT_TRY_FIND_PACKAGE_MODE=ALWAYS was tried first, to make this resolve
# to an already-installed apriltag::apriltag instead of a fresh FetchContent build - see
# CMakePresets.json's linux-base preset for why that doesn't work today.)
#
# When LUMEN_WITH_VULKAN_APRILTAG is OFF, vkapriltag's subdirectory is never added, so this
# falls back to find_package(apriltag) against whatever's installed (install-deps.sh's own
# from-source patched build on Linux, or a plain apriltag install anywhere else) - there is
# nothing else in the build that could collide with it in that case.
if(LUMEN_WITH_VULKAN_APRILTAG)
    find_package(Vulkan REQUIRED)
    set(VKAPRILTAG_BUILD_APPS OFF CACHE BOOL "" FORCE)
    set(VKAPRILTAG_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/vkapriltag/apriltags_vulkan" vkapriltag)
    # vkapriltag's own library/CMakeLists.txt creates a plain `apriltag` target via
    # FetchContent_MakeAvailable(apriltag), not a namespaced `apriltag::apriltag` alias - link
    # against exactly what it produced, not a name we'd have to guess or re-derive.
    target_link_libraries(LumenCore PRIVATE apriltag vkapriltag::vkapriltag Vulkan::Vulkan)
else()
    find_package(apriltag REQUIRED CONFIG)
    target_link_libraries(LumenCore PRIVATE apriltag::apriltag)
endif()

# --- Optional: codec-stereo (hardware-motion-vector stereo depth submodule) -------------------
if(LUMEN_WITH_CODEC_STEREO)
    set(CS_ENABLE_REF_SAD ON CACHE BOOL "" FORCE)
    # CS_ENABLE_LAVC needs ffmpeg - only turn it on where LumenFFmpeg.cmake actually found it,
    # rather than unconditionally (which would hard-fail codec-stereo's own configure on a
    # machine without ffmpeg dev packages, even though CS_ENABLE_REF_SAD alone works fine).
    include(LumenFFmpeg)
    if(LUMEN_FFMPEG_FOUND)
        set(CS_ENABLE_LAVC ON CACHE BOOL "" FORCE)
    else()
        set(CS_ENABLE_LAVC OFF CACHE BOOL "" FORCE)
    endif()
    # CS_ENABLE_RKMPP (the buggy KEY_MOTION_INFO readback backend) is deliberately never
    # enabled - see StereoDepthBackendKind.h. CS_ENABLE_RKMPP_HWENC is aarch64-only; gated the
    # same way install-deps.sh gated it (arch check + pkg-config probe), reproduced here since
    # add_subdirectory means codec-stereo's own configure now happens as part of this one.
    set(CS_ENABLE_RKMPP OFF CACHE BOOL "" FORCE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(LUMEN_ROCKCHIP_MPP QUIET rockchip_mpp)
        endif()
        if(LUMEN_ROCKCHIP_MPP_FOUND)
            set(CS_ENABLE_RKMPP_HWENC ON CACHE BOOL "" FORCE)
        else()
            message(WARNING "rockchip_mpp not found - building codec-stereo without CS_ENABLE_RKMPP_HWENC")
            set(CS_ENABLE_RKMPP_HWENC OFF CACHE BOOL "" FORCE)
        endif()
    else()
        set(CS_ENABLE_RKMPP_HWENC OFF CACHE BOOL "" FORCE)
    endif()
    set(CS_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(CS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    # Its harness pkg-configs the system opencv4 package, which collides with this project's own
    # OpenCV 5.0 (its own CMakeLists carries a comment about exactly this) - not needed for
    # LumenCore anyway.
    set(CS_BUILD_HARNESS OFF CACHE BOOL "" FORCE)
    set(CS_BUILD_PIPELINE ON CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/codec-stereo" codec-stereo)
    # codec_stereo's own CMakeLists already declares `target_link_libraries(codec_stereo PUBLIC
    # m)` on UNIX and PUBLIC Threads::Threads - ordinary CMake target linking propagates both
    # automatically, unlike the old vcxproj's static-archive link which needed `m` listed by
    # hand (LumenCore.vcxproj's own comment on this explains why: a plain .a link doesn't
    # propagate a dependency's PUBLIC requirements the way a real CMake target does).
    target_link_libraries(LumenCore PRIVATE codec_stereo)
endif()

# --- Optional: ONNX Runtime object detection backend -------------------------------------------
if(LUMEN_WITH_ONNX)
    find_package(OnnxRuntime REQUIRED)
    target_link_libraries(LumenCore PRIVATE OnnxRuntime::onnxruntime)
endif()

# --- Optional: NetworkTables 4 publishing -------------------------------------------------------
if(LUMEN_WITH_NT4)
    find_package(Ntcore REQUIRED)
    target_link_libraries(LumenCore PRIVATE Ntcore::ntcore)
endif()

# --- Optional: WebRTC live view -----------------------------------------------------------------
if(LUMEN_WITH_WEBRTC)
    find_package(LibDataChannel REQUIRED)
    include(LumenFFmpeg)
    if(NOT LUMEN_FFMPEG_FOUND)
        message(FATAL_ERROR "LUMEN_WITH_WEBRTC=ON but ffmpeg (libavcodec/avformat/avutil/swscale) was not found via pkg-config")
    endif()
    target_link_libraries(LumenCore PRIVATE LibDataChannel::LibDataChannel Lumen::ffmpeg)
endif()

# --- Optional: Rockchip NPU object detection (aarch64 only; RknnDetectionBackend not yet
# written - ROADMAP.md Phase 6 - so this currently only affects what's available to LINK, not
# what code exists) --------------------------------------------------------------------------
if(LUMEN_WITH_RKNN)
    find_package(RKNN REQUIRED)
    target_link_libraries(LumenCore PRIVATE RKNN::rknn)
endif()

# --- Packaging: collect every transitive runtime dependency next to the built library ---------
# Linux has rpath/LD_LIBRARY_PATH; Windows has neither, so every DLL LumenCore depends on
# (OpenCV, ONNX Runtime, ntcore, ffmpeg, libdatachannel, ...) must sit next to it for the .NET
# host process to find it. A POST_BUILD copy rather than install(FILES $<TARGET_RUNTIME_DLLS:...>)
# deliberately: install() only populates CMAKE_INSTALL_PREFIX after an explicit `cmake --install`
# step, which Server.csproj has no reason to know about or trigger - `dotnet build` triggering
# `cmake --build` alone should be enough, matching how the old vcxproj-based build never needed a
# separate install step either. $<TARGET_FILE_DIR:LumenCore> already resolves to the correct
# per-configuration output directory for the multi-config generators every preset uses (Ninja
# Multi-Config, Visual Studio), so this needs no per-config handling of its own. Windows-only:
# on Linux, $<TARGET_RUNTIME_DLLS:...> is simply empty (the .so's own DT_NEEDED entries plus the
# system loader already handle it), and `cmake -E copy_if_different` with no source arguments at
# all is a malformed invocation, not a harmless no-op - confirmed the hard way (it prints
# cmake -E's own usage help instead of copying anything, rather than silently doing nothing).
# Server.csproj globs this directory rather than hardcoding one filename (see its own comments)
# precisely so this list doesn't have to be kept in sync by hand on the C# side.
if(WIN32)
    add_custom_command(TARGET LumenCore POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_RUNTIME_DLLS:LumenCore>"
                "$<TARGET_FILE_DIR:LumenCore>"
        COMMAND_EXPAND_LISTS
    )
endif()

# --- The SWIG/native pairing stamp -------------------------------------------------------------
# Server.csproj's CheckLumenCoreExists target checks this file matches the native library it's
# about to bundle, not just that the .so/.dll merely exists - the case that check alone can't
# catch is C# bindings generated by one configure (one SWIG version, one define set) paired with
# a native library built by a DIFFERENT one, which happens easily in a multi-preset (Windows/
# WSL/Pi) workflow and previously had no detection at all beyond a runtime
# EntryPointNotFoundException.
string(REPLACE ";" "," _lumen_stamp_defines "${LUMEN_SWIG_DEFINES}")
file(WRITE "${CMAKE_SOURCE_DIR}/Server/Interop/.swig-stamp"
    "swig=${SWIG_VERSION}\nmodule=LumenCore\ndefines=${_lumen_stamp_defines}\npreset=${CMAKE_BUILD_TYPE}-${CMAKE_SYSTEM_PROCESSOR}\n"
)
