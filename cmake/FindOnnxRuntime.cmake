#[[
  Finds the official prebuilt ONNX Runtime distribution (never built from source - see
  install-deps.sh's build_onnxruntime, which just downloads and unpacks the GitHub release
  tarball/zip for the host architecture).

  Variables consulted (optional):
    ONNXRUNTIME_ROOT - root of an unpacked onnxruntime-{linux,win}-{x64,aarch64}-<ver> release
                        (a directory containing include/ and lib/). On Linux, plain /usr/local
                        (already a default search path) is normally enough, since that's where
                        install-deps.sh copies the release's include/lib contents.

  Provides:
    OnnxRuntime::onnxruntime - INTERFACE target: include dir + the runtime lib linked
    ONNXRUNTIME_FOUND
]]

find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS "${ONNXRUNTIME_ROOT}/include"
)

find_library(ONNXRUNTIME_LIBRARY
    NAMES onnxruntime
    HINTS "${ONNXRUNTIME_ROOT}/lib"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OnnxRuntime
    REQUIRED_VARS ONNXRUNTIME_LIBRARY ONNXRUNTIME_INCLUDE_DIR
)

if(ONNXRUNTIME_FOUND AND NOT TARGET OnnxRuntime::onnxruntime)
    if(WIN32)
        # A plain INTERFACE target linking the .lib path (the Linux-style approach below)
        # leaves CMake with no idea a runtime .dll exists at all - $<TARGET_RUNTIME_DLLS:...>
        # (LumenCore/CMakeLists.txt's POST_BUILD copy step) only enumerates DLLs from targets
        # that carry IMPORTED_LOCATION, so onnxruntime.dll silently never made it next to
        # LumenCore.dll until this was declared as a real SHARED IMPORTED target instead -
        # confirmed missing the first time this was run natively on Windows.
        get_filename_component(_onnxruntime_lib_dir "${ONNXRUNTIME_LIBRARY}" DIRECTORY)
        add_library(OnnxRuntime::onnxruntime SHARED IMPORTED)
        set_target_properties(OnnxRuntime::onnxruntime PROPERTIES
            IMPORTED_IMPLIB "${ONNXRUNTIME_LIBRARY}"
            IMPORTED_LOCATION "${_onnxruntime_lib_dir}/onnxruntime.dll"
        )
    else()
        add_library(OnnxRuntime::onnxruntime INTERFACE IMPORTED)
        target_link_libraries(OnnxRuntime::onnxruntime INTERFACE "${ONNXRUNTIME_LIBRARY}")
    endif()
    target_include_directories(OnnxRuntime::onnxruntime INTERFACE "${ONNXRUNTIME_INCLUDE_DIR}")
endif()

mark_as_advanced(ONNXRUNTIME_INCLUDE_DIR ONNXRUNTIME_LIBRARY)
