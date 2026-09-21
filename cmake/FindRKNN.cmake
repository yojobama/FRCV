#[[
  Finds the Rockchip RKNN runtime (the RK3588 NPU SDK: rknn_api.h + librknnrt.so). aarch64-only
  by construction - LumenFeatures.cmake already refuses LUMEN_WITH_RKNN=ON on any other
  CMAKE_SYSTEM_PROCESSOR, so this module is never even consulted on Windows or x64 Linux.

  install-deps.sh's fetch_rknn() installs both under the standard /usr/local prefix (already a
  default CMake search path on Linux), matching every other from-source/fetched dependency in
  this project - no ROOT hint variable is provided or needed, unlike FindNtcore/FindOnnxRuntime,
  since there is no Windows case to support here at all.

  Note (see docs/history/IMPLEMENTATION_PLAN.md risk 2, and ROADMAP.md's note on kernel/runtime
  version skew on the current bench board): librknnrt.so being *found* here says nothing about
  whether its version matches the running kernel's rknpu driver - that can only be confirmed by
  actually calling rknn_init() against a real model, which is a runtime concern, not a configure-
  time one.

  Provides:
    RKNN::rknn    - INTERFACE target: include dir + librknnrt.so linked
    RKNN_FOUND
]]

find_path(RKNN_INCLUDE_DIR NAMES rknn_api.h)
find_library(RKNN_LIBRARY NAMES rknnrt)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RKNN
    REQUIRED_VARS RKNN_LIBRARY RKNN_INCLUDE_DIR
)

if(RKNN_FOUND AND NOT TARGET RKNN::rknn)
    add_library(RKNN::rknn INTERFACE IMPORTED)
    target_include_directories(RKNN::rknn INTERFACE "${RKNN_INCLUDE_DIR}")
    target_link_libraries(RKNN::rknn INTERFACE "${RKNN_LIBRARY}")
endif()

mark_as_advanced(RKNN_INCLUDE_DIR RKNN_LIBRARY)
