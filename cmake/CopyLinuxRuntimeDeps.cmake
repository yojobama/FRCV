#[[
  Script-mode helper (invoked via `cmake -D EXECUTABLE=... -D DEST_DIR=... -P` from LumenCore/
  CMakeLists.txt's POST_BUILD step) that copies every runtime .so LumenCore transitively needs
  from install-deps.sh's own from-source/vendored install locations (/usr/local, /opt/lumenvision-
  ffmpeg) next to the built library - the Linux equivalent of that same POST_BUILD step's
  Windows-only $<TARGET_RUNTIME_DLLS:LumenCore> copy.

  Deliberately narrow include filter (only /usr/local/** and /opt/lumenvision-ffmpeg/**) rather
  than an ever-growing glibc/libstdc++ exclude blocklist: anything install-deps.sh built from
  source or vendored (OpenCV, AprilTag, ffmpeg-rockchip, librga, rockchip_mpp, ONNX Runtime,
  librknnrt, libdatachannel, ntcore) lives under one of those two prefixes and genuinely needs
  bundling (dpkg/apt has no package that "owns" them, so a .deb's dependency graph can't express
  needing them). Anything the dynamic linker resolves from elsewhere (glibc, libstdc++, apt-
  installed libssl/libavahi/etc.) is either toolchain-ABI-sensitive (must NOT be bundled - a
  mismatched libc is a much worse failure mode than a missing one) or a real apt package the
  target image is expected to already have - see scripts/build-deb.sh's own Depends: list for
  those, this script isn't responsible for generating it.

  Uses file(GET_RUNTIME_DEPENDENCIES) (CMake 3.21+), the modern, cross-platform-correct command
  for exactly this - it resolves the full transitive closure via the platform's own dependency
  walker (objdump on Linux), not a one-level ldd of just the top-level library.
]]

if(NOT DEFINED EXECUTABLE)
    message(FATAL_ERROR "CopyLinuxRuntimeDeps.cmake requires -D EXECUTABLE=<path to libLumenCore.so>")
endif()
if(NOT DEFINED DEST_DIR)
    message(FATAL_ERROR "CopyLinuxRuntimeDeps.cmake requires -D DEST_DIR=<directory to copy resolved deps into>")
endif()

# DIRECTORIES matters beyond just "where EXECUTABLE itself lives": the walk is transitive (it
# also resolves EXECUTABLE's dependencies' OWN dependencies, e.g. libavcodec.so's own need for
# libswresample.so), and a from-source lib with no embedded RPATH of its own (plain ffmpeg
# `make install` doesn't set one) only gets resolved one level deep via libLumenCore.so's own
# RPATH - its own further transitive deps then fall back to default system search paths, which
# deliberately exclude /opt/lumenvision-ffmpeg (see this file's own header comment on why it's
# outside ldconfig's search path) and so came back UNRESOLVED here even though the file
# genuinely exists - confirmed the hard way (libswresample.so.5, pulled in transitively by
# ffmpeg's built-in opus decoder, silently skipped from the stage/Release/ copy, which then
# broke the separate LumenCoreTests link since Lumen::ffmpeg's rpath is PRIVATE to LumenCore and
# doesn't propagate to a test executable that only links the .so).
file(GET_RUNTIME_DEPENDENCIES
    LIBRARIES "${EXECUTABLE}"
    DIRECTORIES "/usr/local/lib" "/opt/lumenvision-ffmpeg/lib"
    RESOLVED_DEPENDENCIES_VAR _resolvedDeps
    UNRESOLVED_DEPENDENCIES_VAR _unresolvedDeps
)

if(_unresolvedDeps)
    message(WARNING "CopyLinuxRuntimeDeps.cmake: could not resolve: ${_unresolvedDeps} (fine if these are optional/dlopen-only - not copied)")
endif()

foreach(_dep ${_resolvedDeps})
    if(_dep MATCHES "^/usr/local/" OR _dep MATCHES "^/opt/lumenvision-ffmpeg/")
        file(COPY "${_dep}" DESTINATION "${DEST_DIR}" FOLLOW_SYMLINK_CHAIN)
    endif()
endforeach()
