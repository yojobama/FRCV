#[[
  Script-mode helper (invoked via `cmake -D EXECUTABLE=... -D DEST_DIR=... -P` from LumenCore/
  CMakeLists.txt's POST_BUILD step) that copies every runtime .so LumenCore transitively needs
  from install-deps.sh's own from-source/vendored install locations (/usr/local, /opt/lumenvision-
  ffmpeg) next to the built library - the Linux equivalent of that same POST_BUILD step's
  Windows-only $<TARGET_RUNTIME_DLLS:LumenCore> copy.

  Deliberately narrow include filter (/usr/local/**, /opt/lumenvision-ffmpeg/**, plus a small
  explicit allowlist below) rather than an ever-growing glibc/libstdc++ exclude blocklist:
  anything install-deps.sh built from source or vendored (OpenCV, AprilTag, ffmpeg-rockchip,
  librga, rockchip_mpp, ONNX Runtime, librknnrt, libdatachannel, ntcore) lives under one of those
  two prefixes and genuinely needs bundling (dpkg/apt has no package that "owns" them, so a .deb's
  dependency graph can't express needing them). Anything the dynamic linker resolves from
  elsewhere (glibc, libstdc++, apt-installed libssl/libavahi/etc.) is either toolchain-ABI-
  sensitive (must NOT be bundled - a mismatched libc is a much worse failure mode than a missing
  one) or a real apt package the target image is expected to already have - see
  scripts/build-deb.sh's own Depends: list for those, this script isn't responsible for
  generating it.

  The explicit allowlist exists for a real, confirmed exception to that second case: OpenCV's
  imgcodecs module and ffmpeg-rockchip's own build both dynamically link a handful of apt-
  installed system codec libraries (libjpeg/libpng/libtiff/libwebp, libx264, and libtiff's/
  libwebp's own further backends - libdeflate/libjbig/libLerc for TIFF, libsharpyuv for WebP) at
  BUILD time on the ubuntu-24.04-arm CI runner - these are genuinely self-contained image/media
  codec libraries, not toolchain-ABI-sensitive like libc/libstdc++, so bundling them is safe. But
  unlike glibc/libssl/libavahi, they're NOT a safe Depends: away from working on the Debian 13
  trixie target image either - confirmed the hard way on real hardware: Ubuntu's libjpeg-turbo8
  ships SONAME 8 where Debian ships SONAME 62 for the same library, and libx264's SONAME bumps
  with nearly every build, so a Depends: on either would either not resolve at all or resolve to a
  genuinely different, incompatible file. Bundling sidesteps the cross-distro mismatch entirely,
  the same way ffmpeg-rockchip's own libs already are. This full list was confirmed complete by
  computing the actual transitive NEEDED closure across every .so in a real built .deb (readelf
  -d, cross-referenced against what the bundle itself provides) - not discovered one crash at a
  time, though it took two rounds of real hardware boot-testing to get there.

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

# Basename-anchored, not path-anchored (these live under a standard system multiarch path, e.g.
# /usr/lib/aarch64-linux-gnu/, which is deliberately NOT matched by the two prefix checks above).
set(_allowlistedSystemLibs
    "^libjpeg\\.so"
    "^libpng16\\.so"
    "^libtiff\\.so"
    "^libwebp\\.so"
    "^libwebpdemux\\.so"
    "^libwebpmux\\.so"
    "^libx264\\.so"
    "^libLerc\\.so"
    "^libdeflate\\.so"
    "^libjbig\\.so"
    "^libsharpyuv\\.so"
)

foreach(_dep ${_resolvedDeps})
    get_filename_component(_depName "${_dep}" NAME)
    set(_shouldCopy FALSE)
    if(_dep MATCHES "^/usr/local/" OR _dep MATCHES "^/opt/lumenvision-ffmpeg/")
        set(_shouldCopy TRUE)
    else()
        foreach(_pattern ${_allowlistedSystemLibs})
            if(_depName MATCHES "${_pattern}")
                set(_shouldCopy TRUE)
            endif()
        endforeach()
    endif()
    if(_shouldCopy)
        file(COPY "${_dep}" DESTINATION "${DEST_DIR}" FOLLOW_SYMLINK_CHAIN)
    endif()
endforeach()
