#[[
  ROADMAP.md Phase 5: a real ctest suite, replacing the dead pre-CMake UnitTests tree (which
  compiled one file with every line commented out - zero actual coverage; not worth porting).

  Catch2 via FetchContent rather than an installed system package: keeps the test toolchain
  identical across WSL/Windows/the Pi with no extra install-deps.sh step, matching how OpenCV/
  apriltag/etc. are already handled for this project's other dependencies where a matching
  version isn't reliably available as a distro package.
]]

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(LumenCoreTests
    test_source_result.cpp
    test_source_sink_lifecycle.cpp
    test_yolo_postprocess.cpp
    test_camera_v4l2_hitl.cpp
    test_pixel_unpack.cpp
    test_roi_source.cpp
    test_driver_mode.cpp
    test_snapshot.cpp
    test_multitag_pnp.cpp
    test_sgbm_synthetic_disparity.cpp
)

if(LUMEN_WITH_ONNX)
    # same PRIVATE-compile-definition situation as LUMEN_WITH_RKNN below -
    # OnnxDetectionBackend.h's own #ifdef LUMEN_WITH_ONNX guard needs this define visible to the
    # test's own translation unit too. Genuinely local (unlike the RKNN/CODEC_STEREO hitl tests
    # right below): ONNX Runtime's CPU execution provider needs no RK3588 hardware, and ONNX is
    # on by default everywhere this project builds.
    #
    # Also needs a direct link against OnnxRuntime::onnxruntime, unlike RKNN/CODEC_STEREO's own
    # blocks below - onnxruntime_cxx_api.h's C++ wrapper classes (Ort::Env/Session/MemoryInfo,
    # used as OnnxDetectionBackend's private members) are header-only, so merely including
    # OnnxDetectionBackend.h from this test's own translation unit pulls in real (not just
    # declared) code that calls into the C API directly - confirmed the hard way: without this,
    # the link fails with "undefined reference to OrtGetApiBase" even though this test never
    # calls any Ort:: API itself. LumenCore already resolves OnnxRuntime via find_package in its
    # own CMakeLists.txt, but that link is PRIVATE (deliberately, so consumers of the .so don't
    # need onnxruntime headers) and so doesn't propagate here.
    find_package(OnnxRuntime REQUIRED)
    target_sources(LumenCoreTests PRIVATE test_onnx_detection.cpp)
    target_compile_definitions(LumenCoreTests PRIVATE LUMEN_WITH_ONNX)
    target_link_libraries(LumenCoreTests PRIVATE OnnxRuntime::onnxruntime)
endif()

if(LUMEN_WITH_RKNN)
    # only buildable at all where RknnDetectionBackend.h itself has real content - see that
    # header's own #ifdef LUMEN_WITH_RKNN guard. LumenCore's own LUMEN_WITH_RKNN compile
    # definition is PRIVATE to its own target (deliberately - see LumenCore/CMakeLists.txt), so a
    # test including RknnDetectionBackend.h directly needs the same define applied here too, or
    # the #ifdef silently strips the header's entire contents for this translation unit.
    target_sources(LumenCoreTests PRIVATE test_rknn_detection_hitl.cpp)
    target_compile_definitions(LumenCoreTests PRIVATE LUMEN_WITH_RKNN)
endif()

if(LUMEN_WITH_VULKAN_APRILTAG)
    # same "header-only real code, not just declarations, reaches this test's own translation
    # unit" situation as LUMEN_WITH_ONNX above - VkApriltagBackend.h includes real vkapriltag
    # headers (<vkapriltag/TagDecoder.h>), not just forward declarations, so this needs the same
    # include dirs and link libraries LumenCore/CMakeLists.txt itself uses for exactly that
    # target (apriltag, vkapriltag::vkapriltag, Vulkan::Vulkan) - that linkage is PRIVATE to
    # LumenCore and doesn't propagate to an external target like this one.
    find_package(Vulkan REQUIRED)
    target_sources(LumenCoreTests PRIVATE test_apriltag_backend_agreement_hitl.cpp)
    target_compile_definitions(LumenCoreTests PRIVATE LUMEN_WITH_VULKAN_APRILTAG)
    target_link_libraries(LumenCoreTests PRIVATE apriltag vkapriltag::vkapriltag Vulkan::Vulkan)
endif()

if(LUMEN_WITH_NT4)
    # same PRIVATE-compile-definition situation as LUMEN_WITH_RKNN above - NetworkTablesSink.h's
    # own #ifdef LUMEN_WITH_NT4 guard needs this define visible to the test's own translation unit
    # too. Also needs a direct link against Ntcore::ntcore, same "PRIVATE on LumenCore doesn't
    # propagate to an external target" situation as LUMEN_WITH_ONNX/LUMEN_WITH_VULKAN_APRILTAG
    # above - this test's own translation unit includes NetworkTablesSink.h directly (real ntcore
    # types like nt::NetworkTableInstance appear in the constructor it calls, not just forward
    # declarations), so it needs ntcore's headers/libs visible here too.
    find_package(Ntcore REQUIRED)
    target_sources(LumenCoreTests PRIVATE test_networktables_nt4_e2e.cpp)
    target_compile_definitions(LumenCoreTests PRIVATE LUMEN_WITH_NT4 LUMEN_VERSION_STRING="${PROJECT_VERSION}")
    target_link_libraries(LumenCoreTests PRIVATE Ntcore::ntcore)
endif()

if(LUMEN_WITH_MPP_JPEG)
    # same "header-only real code, not just declarations" situation as the other optional-backend
    # blocks above - MppJpegDecoder.h's #ifdef LUMEN_WITH_MPP_JPEG guard needs this define visible
    # to this test's own translation unit too, and it needs the real rockchip_mpp headers/libs
    # LumenCore/CMakeLists.txt resolves for its own (PRIVATE) use of them.
    target_sources(LumenCoreTests PRIVATE test_mpp_jpeg_decoder_hitl.cpp)
    target_compile_definitions(LumenCoreTests PRIVATE LUMEN_WITH_MPP_JPEG)
    target_include_directories(LumenCoreTests PRIVATE ${LUMEN_ROCKCHIP_MPP_INCLUDE_DIRS})
    target_link_libraries(LumenCoreTests PRIVATE ${LUMEN_ROCKCHIP_MPP_LIBRARIES})
endif()

if(LUMEN_WITH_CODEC_STEREO)
    # same PRIVATE-compile-definition situation as LUMEN_WITH_RKNN above - CodecStereoBackend.h's
    # own #ifdef LUMEN_WITH_CODEC_STEREO guard needs this define visible to the test's own
    # translation unit too. No extra include dir/link needed: CodecStereoBackend.h only forward-
    # declares cs_context (never includes codec-stereo's own cs.h), and the actual cs_* calls
    # live inside CodecStereoBackend.cpp, already compiled into LumenCore itself.
    target_sources(LumenCoreTests PRIVATE
        test_codec_stereo_rkmpp_hitl.cpp
        test_codec_stereo_lavc_synthetic_disparity.cpp
    )
    target_compile_definitions(LumenCoreTests PRIVATE LUMEN_WITH_CODEC_STEREO)
endif()

target_link_libraries(LumenCoreTests PRIVATE
    LumenCore
    Catch2::Catch2WithMain
)

# LumenCore's own headers aren't installed anywhere (this project has no `cmake --install` step
# in its normal workflow - see LumenCore/CMakeLists.txt's packaging comment) so tests include
# them straight from the source tree, the same way LumenCore/CMakeLists.txt itself does.
target_include_directories(LumenCoreTests PRIVATE "${CMAKE_SOURCE_DIR}/LumenCore")

# an absolute path, not a relative one - catch_discover_tests' default WORKING_DIRECTORY is the
# test executable's own output directory, not this source directory, so a relative path to
# data/coco_80_labels.txt would silently fail to resolve.
target_compile_definitions(LumenCoreTests PRIVATE LUMEN_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# grayimage.pgm is tracked in the vkapriltag submodule's own git history regardless of whether
# LUMEN_WITH_VULKAN_APRILTAG is actually on - defined unconditionally (unlike that feature's own
# LUMEN_WITH_VULKAN_APRILTAG define above) so both the CPU-vs-Vulkan agreement test and the NT4
# end-to-end test can use the same real fixture without depending on each other's feature gate.
target_compile_definitions(LumenCoreTests PRIVATE LUMEN_VKAPRILTAG_SAMPLE_DIR="${CMAKE_SOURCE_DIR}/third_party/vkapriltag/apriltags_vulkan")

if(MSVC)
    target_compile_definitions(LumenCoreTests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_options(LumenCoreTests PRIVATE /utf-8)
endif()

# Catch2's own CMake integration module (Catch.cmake, providing catch_discover_tests) isn't on
# CMAKE_MODULE_PATH by default when pulled in via FetchContent rather than find_package/install -
# it ships under the fetched source tree's own extras/ directory.
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(LumenCoreTests)
