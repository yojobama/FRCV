#[[
  ROADMAP.md Phase 8/E7: a small standalone timing tool, off by default like tests/ - every
  ci-*/wsl-*/pi-* preset that wants it turns LUMEN_BUILD_TOOLS on explicitly.
]]

add_executable(lumen-bench main.cpp)

set_target_properties(lumen-bench PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(lumen-bench PRIVATE LumenCore)

# same "LumenCore has no `cmake --install` step, so consumers include straight from the source
# tree" reasoning as tests/CMakeLists.txt.
target_include_directories(lumen-bench PRIVATE "${CMAKE_SOURCE_DIR}/LumenCore")

# same real fixtures test_apriltag_backend_agreement_hitl.cpp / test_onnx_detection.cpp already
# use - reused here rather than duplicating checked-in files a second time under tools/.
target_compile_definitions(lumen-bench PRIVATE
    LUMEN_VKAPRILTAG_SAMPLE_DIR="${CMAKE_SOURCE_DIR}/third_party/vkapriltag/apriltags_vulkan"
    LUMEN_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data"
)

if(LUMEN_WITH_VULKAN_APRILTAG)
    # same PRIVATE-link-doesn't-propagate situation as tests/CMakeLists.txt's own identical block
    # - VkApriltagBackend.h pulls in real vkapriltag headers, not just forward declarations.
    find_package(Vulkan REQUIRED)
    target_compile_definitions(lumen-bench PRIVATE LUMEN_WITH_VULKAN_APRILTAG)
    target_link_libraries(lumen-bench PRIVATE apriltag vkapriltag::vkapriltag Vulkan::Vulkan)
endif()

if(LUMEN_WITH_CODEC_STEREO)
    target_compile_definitions(lumen-bench PRIVATE LUMEN_WITH_CODEC_STEREO)
endif()

if(LUMEN_WITH_ONNX)
    # same header-only-C++-API situation as tests/CMakeLists.txt's own identical block -
    # onnxruntime_cxx_api.h's wrapper classes are inline-defined, so this needs a direct link
    # even though this file never calls any Ort:: API itself.
    find_package(OnnxRuntime REQUIRED)
    target_compile_definitions(lumen-bench PRIVATE LUMEN_WITH_ONNX)
    target_link_libraries(lumen-bench PRIVATE OnnxRuntime::onnxruntime)
endif()

if(LUMEN_WITH_RKNN)
    target_compile_definitions(lumen-bench PRIVATE LUMEN_WITH_RKNN)
endif()

if(MSVC)
    target_compile_definitions(lumen-bench PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_options(lumen-bench PRIVATE /utf-8)
endif()
