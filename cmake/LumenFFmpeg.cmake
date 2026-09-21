#[[
  ffmpeg (libavcodec/libavformat/libavutil/libswscale) for WebRTCSink and CodecStereoBackend's
  CS_ENABLE_LAVC path, via pkg-config uniformly on every platform rather than two separate
  find-path codepaths:

  - Linux: the system pkg-config finds install-deps.sh's apt-installed dev packages directly.
  - Windows: vcpkg's own toolchain integration registers its pkgconf.exe and sets
    PKG_CONFIG_PATH to the triplet's installed .pc files automatically once
    CMAKE_TOOLCHAIN_FILE points at vcpkg.cmake (confirmed present on the Windows dev box:
    ffmpeg:x64-windows and pkgconf:x64-windows are already built) - no separate Windows-only
    Find module needed.

  Note the vcpkg ffmpeg:x64-windows port pulled in here is LGPL with no x264 feature - it has
  no encoder WebRTCSink's own default (encoderName = "libx264", WebRTCSink.h) can use. That is
  exactly why LUMEN_WITH_WEBRTC defaults OFF on the windows-x64 presets (see
  cmake/LumenFeatures.cmake's defaults and CMakePresets.json) rather than something this module
  tries to paper over - a team that wants WebRTC on Windows needs a GPL ffmpeg build providing
  libx264, which is a deliberate opt-in, not a default.

  Provides:
    Lumen::ffmpeg - INTERFACE target aggregating avcodec/avformat/avutil/swscale
    LUMEN_FFMPEG_FOUND
]]

find_package(PkgConfig REQUIRED)

pkg_check_modules(LUMEN_AVCODEC  IMPORTED_TARGET libavcodec)
pkg_check_modules(LUMEN_AVFORMAT IMPORTED_TARGET libavformat)
pkg_check_modules(LUMEN_AVUTIL   IMPORTED_TARGET libavutil)
pkg_check_modules(LUMEN_SWSCALE  IMPORTED_TARGET libswscale)

set(LUMEN_FFMPEG_FOUND FALSE)
if(LUMEN_AVCODEC_FOUND AND LUMEN_AVFORMAT_FOUND AND LUMEN_AVUTIL_FOUND AND LUMEN_SWSCALE_FOUND)
    set(LUMEN_FFMPEG_FOUND TRUE)
    if(NOT TARGET Lumen::ffmpeg)
        add_library(Lumen::ffmpeg INTERFACE IMPORTED)
        target_link_libraries(Lumen::ffmpeg INTERFACE
            PkgConfig::LUMEN_AVCODEC PkgConfig::LUMEN_AVFORMAT
            PkgConfig::LUMEN_AVUTIL PkgConfig::LUMEN_SWSCALE
        )
    endif()
elseif(LUMEN_WITH_WEBRTC OR LUMEN_WITH_CODEC_STEREO)
    message(WARNING
        "ffmpeg (libavcodec/libavformat/libavutil/libswscale) not found via pkg-config, but "
        "LUMEN_WITH_WEBRTC or LUMEN_WITH_CODEC_STEREO's CS_ENABLE_LAVC path needs it.")
endif()
