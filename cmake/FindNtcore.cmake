#[[
  Finds WPILib's ntcore + wpiutil + wpinet (the NT4 client library) and exposes them as ONE
  interface target, Ntcore::ntcore.

  wpinet is the point of this module, not an afterthought: LumenCore.vcxproj's LibraryDependencies
  (`apriltag;opencv_core;...;ntcore;wpiutil;onnxruntime;...`) never listed wpinet explicitly, and
  it only ever resolved on Linux because libntcore.so's own DT_NEEDED entry pulls libwpinet.so in
  transitively (confirmed: `readelf -d libntcore.so` lists it). Windows has no ELF-style transitive
  resolution, so a Windows build silently missing wpinet would fail to link with undefined
  references the moment anything in NetworkTablesSink.cpp actually touches networking code, not at
  the include site. Linking Ntcore::ntcore always pulls all three, on every platform, so this class
  of bug can't recur.

  Variables consulted (all optional):
    WPILIB_ROOT   - root of an unpacked WPILib artifact tree (a directory containing include/ and
                    (lib|bin)/). Set this for the Windows/prebuilt-zip case; on Linux, plain
                    /usr/local (already a default CMake search path) is normally enough on its own
                    since that's where install-deps.sh's `--with-nt4` step installs them.

  Provides:
    Ntcore::ntcore        - INTERFACE target: include dirs + ntcore, wpiutil, wpinet all linked
    NTCORE_FOUND
]]

find_path(NTCORE_INCLUDE_DIR
    NAMES ntcore_cpp.h
    HINTS "${WPILIB_ROOT}/include"
)

find_path(WPINET_INCLUDE_DIR
    NAMES wpinet/uv/Loop.h
    HINTS "${WPILIB_ROOT}/include"
)

find_path(WPIUTIL_INCLUDE_DIR
    NAMES wpi/json.h
    HINTS "${WPILIB_ROOT}/include"
)

# On Windows, WPILib's prebuilt zips split each library into an import .lib (linked at build
# time) and a runtime .dll (needed at run time, discovered separately via
# $<TARGET_RUNTIME_DLLS:...> once this target is linked into LumenCore - see LumenCore/CMakeLists.txt).
# find_library on Windows finds the .lib; on Linux it finds the .so directly.
find_library(NTCORE_LIBRARY   NAMES ntcore   HINTS "${WPILIB_ROOT}/lib" "${WPILIB_ROOT}/bin")
find_library(WPIUTIL_LIBRARY  NAMES wpiutil  HINTS "${WPILIB_ROOT}/lib" "${WPILIB_ROOT}/bin")
find_library(WPINET_LIBRARY   NAMES wpinet   HINTS "${WPILIB_ROOT}/lib" "${WPILIB_ROOT}/bin")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ntcore
    REQUIRED_VARS NTCORE_LIBRARY WPIUTIL_LIBRARY WPINET_LIBRARY
                   NTCORE_INCLUDE_DIR WPINET_INCLUDE_DIR WPIUTIL_INCLUDE_DIR
)

if(NTCORE_FOUND AND NOT TARGET Ntcore::ntcore)
    if(WIN32)
        # See FindOnnxRuntime.cmake's identical comment: a plain INTERFACE target linking the
        # .lib paths (the Linux-style approach below) leaves $<TARGET_RUNTIME_DLLS:...> with no
        # runtime .dll to find - ntcore.dll/wpinet.dll/wpiutil.dll never made it next to
        # LumenCore.dll until each got its own real SHARED IMPORTED target.
        foreach(_lib NTCORE WPINET WPIUTIL)
            get_filename_component(_dir "${${_lib}_LIBRARY}" DIRECTORY)
            get_filename_component(_name "${${_lib}_LIBRARY}" NAME_WE)
            add_library(Ntcore::${_lib} SHARED IMPORTED)
            set_target_properties(Ntcore::${_lib} PROPERTIES
                IMPORTED_IMPLIB "${${_lib}_LIBRARY}"
                IMPORTED_LOCATION "${_dir}/${_name}.dll"
            )
        endforeach()
        add_library(Ntcore::ntcore INTERFACE IMPORTED)
        target_link_libraries(Ntcore::ntcore INTERFACE Ntcore::NTCORE Ntcore::WPINET Ntcore::WPIUTIL)
    else()
        add_library(Ntcore::ntcore INTERFACE IMPORTED)
        # Order matters for a plain (non-CMake-target) linker line on Linux: ntcore depends on
        # wpiutil/wpinet, so they must come after it. Harmless on Windows either way.
        target_link_libraries(Ntcore::ntcore INTERFACE
            "${NTCORE_LIBRARY}" "${WPINET_LIBRARY}" "${WPIUTIL_LIBRARY}"
        )
    endif()
    target_include_directories(Ntcore::ntcore INTERFACE
        "${NTCORE_INCLUDE_DIR}" "${WPINET_INCLUDE_DIR}" "${WPIUTIL_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(
    NTCORE_INCLUDE_DIR WPINET_INCLUDE_DIR WPIUTIL_INCLUDE_DIR
    NTCORE_LIBRARY WPIUTIL_LIBRARY WPINET_LIBRARY
)
