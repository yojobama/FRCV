#[[
  ffmpeg (libavcodec/libavformat/libavutil/libswscale) for WebRTCSink, RecordSink (the only one of
  the three that actually touches libavformat's muxing API - WebRTCSink RTP-packetizes raw NAL
  units, no container), and CodecStereoBackend's CS_ENABLE_LAVC path, via pkg-config uniformly on
  every platform rather than two separate find-path codepaths:

  - Linux: the system pkg-config finds install-deps.sh's apt-installed dev packages directly.
  - Windows: vcpkg's own toolchain integration registers its pkgconf.exe and sets
    PKG_CONFIG_PATH to the triplet's installed .pc files automatically once
    CMAKE_TOOLCHAIN_FILE points at vcpkg.cmake (confirmed present on the Windows dev box:
    ffmpeg:x64-windows and pkgconf:x64-windows are already built) - no separate Windows-only
    Find module needed.

  Note the plain vcpkg ffmpeg:x64-windows port is LGPL with no x264 feature - it has no encoder
  WebRTCSink's or RecordSink's own default (encoderName = "libx264", WebRTCSink.h/RecordSink.h)
  can use. WebRTC is the standard preview transport for this project (LUMEN_WITH_WEBRTC now
  defaults ON everywhere - see cmake/LumenFeatures.cmake - with MJPEG as StreamView.tsx's
  same-preview fallback for when it breaks, not a parallel always-on alternative), so the Windows
  dev box's vcpkg ffmpeg install was rebuilt with the "gpl" and "x264" features enabled to provide
  it. That is a real, deliberate licensing choice - libx264 is GPL, and any resulting Windows
  binary that links it is GPL-encumbered - carried here explicitly rather than left implicit: a
  from-source ffmpeg build without those features (or a future switch to a non-GPL encoder, e.g.
  openh264) would need WebRTCSink's/RecordSink's encoderName reconfigured accordingly. This
  applies MORE directly to RecordSink than to WebRTCSink: a recording is a file a team downloads
  and redistributes (the whole point, per RecordSink.h's own comment), not an ephemeral RTP
  stream - flagged explicitly to whoever configures a production build, not decided silently here.

  Provides:
    Lumen::ffmpeg - INTERFACE target aggregating avcodec/avformat/avutil/swscale
    LUMEN_FFMPEG_FOUND
]]

find_package(PkgConfig REQUIRED)

# ROADMAP.md Phase 6: a from-source ffmpeg-rockchip build (nyanmisaka/ffmpeg-rockchip, NOT
# upstream FFmpeg - upstream's own --enable-rkmpp is decode-only, confirmed the hard way: it
# ships rkmppdec.c but no encoder, so WebRTCSink's own h264_rkmpp comment silently assumed a
# fork that isn't upstream) lands in its own dedicated prefix, /opt/lumenvision-ffmpeg, rather
# than /usr/local alongside every other from-source dependency this project builds. This is
# deliberate, not an inconsistency: confirmed the hard way on the real board that Debian's
# multiarch ldconfig prioritises /usr/lib/aarch64-linux-gnu (the apt-installed ffmpeg-dev
# package already on this image) over /usr/local/lib for a DUPLICATE SONAME - so installing a
# second libavcodec.so.61 under /usr/local silently loses the race at runtime and produces the
# wrong (non-rkmpp) build with no error, only a "library configuration mismatch" warning easy to
# miss. A dedicated prefix outside ldconfig's default search path, found here via pkg-config and
# resolved at runtime via an explicit rpath below, sidesteps the ambiguity entirely rather than
# fighting it - and leaves the system's own ffmpeg/apt packages completely untouched for
# anything else on the board that depends on them.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "/opt/lumenvision-ffmpeg/lib/pkgconfig")
    set(LUMEN_FFMPEG_DEDICATED_PREFIX "/opt/lumenvision-ffmpeg")
    set(ENV{PKG_CONFIG_PATH} "${LUMEN_FFMPEG_DEDICATED_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    message(STATUS "Using ffmpeg-rockchip (h264_rkmpp encoder + rkrga filters) from ${LUMEN_FFMPEG_DEDICATED_PREFIX}")
endif()

pkg_check_modules(LUMEN_AVCODEC  IMPORTED_TARGET libavcodec)
pkg_check_modules(LUMEN_AVFORMAT IMPORTED_TARGET libavformat)
pkg_check_modules(LUMEN_AVUTIL   IMPORTED_TARGET libavutil)
pkg_check_modules(LUMEN_SWSCALE  IMPORTED_TARGET libswscale)

# pkg-config's .pc files describe compile/link flags, not a runtime .dll location (that
# split doesn't exist on Linux, where the .so IS the runtime artifact) - so
# PkgConfig::LUMEN_AVCODEC etc. carry no IMPORTED_LOCATION on Windows, and
# $<TARGET_RUNTIME_DLLS:...> (LumenCore/CMakeLists.txt's POST_BUILD copy step) can't discover
# avcodec-*.dll etc. through them. vcpkg's own triplet layout is a known quantity - bin/ is
# always a sibling of the lib/ pkg_check_modules just found - so glob it directly instead of
# trying to coax IMPORTED_LOCATION out of a mechanism that fundamentally doesn't carry it.
set(LUMEN_FFMPEG_RUNTIME_DLLS "")
if(WIN32 AND LUMEN_AVCODEC_LIBRARY_DIRS)
    list(GET LUMEN_AVCODEC_LIBRARY_DIRS 0 _lumen_ffmpeg_lib_dir)
    get_filename_component(_lumen_ffmpeg_root "${_lumen_ffmpeg_lib_dir}" DIRECTORY)
    file(GLOB LUMEN_FFMPEG_RUNTIME_DLLS "${_lumen_ffmpeg_root}/bin/av*.dll" "${_lumen_ffmpeg_root}/bin/sw*.dll")
endif()

set(LUMEN_FFMPEG_FOUND FALSE)
if(LUMEN_AVCODEC_FOUND AND LUMEN_AVFORMAT_FOUND AND LUMEN_AVUTIL_FOUND AND LUMEN_SWSCALE_FOUND)
    set(LUMEN_FFMPEG_FOUND TRUE)
    if(NOT TARGET Lumen::ffmpeg)
        add_library(Lumen::ffmpeg INTERFACE IMPORTED)
        target_link_libraries(Lumen::ffmpeg INTERFACE
            PkgConfig::LUMEN_AVCODEC PkgConfig::LUMEN_AVFORMAT
            PkgConfig::LUMEN_AVUTIL PkgConfig::LUMEN_SWSCALE
        )
        if(LUMEN_FFMPEG_DEDICATED_PREFIX)
            # outside ldconfig's default search path by design (see above) - without this,
            # LumenCore.so links fine against the right headers/CFLAGS at compile time but
            # resolves the apt-installed (non-rkmpp) libavcodec.so.61 at process launch instead,
            # since PRIVATE-linking Lumen::ffmpeg alone carries no runtime search path of its own.
            target_link_options(Lumen::ffmpeg INTERFACE "-Wl,-rpath,${LUMEN_FFMPEG_DEDICATED_PREFIX}/lib")
        endif()
    endif()
elseif(LUMEN_WITH_WEBRTC OR LUMEN_WITH_RECORD OR LUMEN_WITH_CODEC_STEREO)
    message(WARNING
        "ffmpeg (libavcodec/libavformat/libavutil/libswscale) not found via pkg-config, but "
        "LUMEN_WITH_WEBRTC, LUMEN_WITH_RECORD, or LUMEN_WITH_CODEC_STEREO's CS_ENABLE_LAVC path needs it.")
endif()
