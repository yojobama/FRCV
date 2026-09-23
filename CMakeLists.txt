cmake_minimum_required(VERSION 3.25)

# 3.25 is set by the lowest floor across the three environments this project builds on: WSL
# "Ubuntu" ships cmake 3.28.3, the Windows dev box has 4.2.3 (VS 2026 bundles 4.3.1-msvc1), and
# the Orange Pi's Debian 13 trixie image ships 3.31.6 - 3.25 clears all of them comfortably while
# still giving FetchContent's FETCHCONTENT_TRY_FIND_PACKAGE_MODE (needs 3.24) and
# $<TARGET_RUNTIME_DLLS:...> (needs 3.21) to work. See ROADMAP.md Phase A2.
project(LumenVision
    VERSION 0.1.0
    DESCRIPTION "PhotonVision-style FRC vision coprocessor - AprilTag/object detection, calibration, WebRTC, NT4, stereo depth"
    LANGUAGES CXX
)
# Bumped by hand for now, kept in step with whatever tag is about to be pushed - the actual
# release pipeline (.github/workflows/release.yml) derives its own version string from the pushed
# git tag itself, not from this file, so the two can drift if this isn't updated at tag time. This
# is what NetworkTablesSink's own ".version" NT4 topic (LumenCore/NetworkTablesSink.cpp) publishes.

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Windows has no rpath/LD_LIBRARY_PATH equivalent - every transitive DLL (OpenCV, ONNX Runtime,
# ntcore, ffmpeg, ...) has to sit next to Server.exe. Collecting build output under one place
# per configuration (mirroring what the old LumenCore.vcxproj's Linux toolset did implicitly:
# bin/$(Platform)/$(Configuration)/) is what makes install(FILES $<TARGET_RUNTIME_DLLS:...>)
# in LumenCore/CMakeLists.txt actually useful to Server.csproj later.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/stage")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/stage")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/stage")

include(LumenFeatures)

# Needed by every static archive linked into LumenCore's shared library (vkapriltag, codec_stereo)
# - set once, here, before either subdirectory is added, so both submodules' own add_subdirectory
# calls inherit it. Missing this is exactly what install-deps.sh's explicit
# -DCMAKE_POSITION_INDEPENDENT_CODE=ON on both submodule builds worked around by hand
# (confirmed the hard way there: "relocation R_X86_64_32S ... can not be used when making a
# shared object" without it).
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(LumenCore)

# Off by default (a real Catch2/ctest suite, tests/, replacing the dead pre-CMake UnitTests
# tree - see ROADMAP.md Phase 5); every ci-* preset turns it on explicitly.
option(LUMEN_BUILD_TESTS "Build the ctest suite (tests/)" OFF)
if(LUMEN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Off by default, same reasoning as LUMEN_BUILD_TESTS - ROADMAP.md Phase 8/E7's standalone
# per-backend timing tool (tools/lumen-bench), not part of the normal build.
option(LUMEN_BUILD_TOOLS "Build standalone tools (tools/lumen-bench)" OFF)
if(LUMEN_BUILD_TOOLS)
    add_subdirectory(tools/lumen-bench)
endif()
