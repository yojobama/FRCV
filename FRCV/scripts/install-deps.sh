#!/usr/bin/env bash
#
# FRCV dependency installer.
#
# Run this ON THE MACHINE THAT COMPILES FRCVLib: inside the WSL2 "Ubuntu" distro for the local
# dev inner loop, or over SSH on the Orange Pi for the ARM64/Remote_GCC configuration. There is
# no CMake/Linux-native build in this project — Visual Studio drives both toolchains remotely;
# this script only prepares the machine underneath VS.
#
# Both targets are expected to be Ubuntu 24.04 (WSL "Ubuntu", and the Orange Pi 5 Plus running
# ubuntu-rockchip v2.4.0 - ubuntu-24.04-preinstalled-server-arm64-orangepi-5-plus), so one script
# covers both; behaviour that differs by CPU architecture is gated on `uname -m`.
#
# Usage:
#   ./install-deps.sh [--check] [--jobs N] [--skip-opencv] [--with-webrtc] [--with-nt4]
#
#   --check         verify what's installed/built and report what's missing; installs nothing
#   --jobs N        parallelism for from-source builds (default: nproc)
#   --skip-opencv   skip the OpenCV 5.0 source build (useful once it's already built and cached)
#   --with-webrtc   also build libdatachannel (phase 6 prerequisite; off by default, it's slow)
#   --with-nt4      also fetch ntcore/wpiutil (phase 3 prerequisite)
#
set -euo pipefail

# ---------------------------------------------------------------------------
# argument parsing
# ---------------------------------------------------------------------------
CHECK_ONLY=0
JOBS="$(nproc 2>/dev/null || echo 4)"
SKIP_OPENCV=0
WITH_WEBRTC=0
WITH_NT4=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --check) CHECK_ONLY=1; shift ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --skip-opencv) SKIP_OPENCV=1; shift ;;
        --with-webrtc) WITH_WEBRTC=1; shift ;;
        --with-nt4) WITH_NT4=1; shift ;;
        -h|--help)
            grep '^#' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

ARCH="$(uname -m)"           # x86_64 or aarch64
PREFIX=/usr/local
BUILD_ROOT="${FRCV_BUILD_ROOT:-$HOME/.frcv-build}"
mkdir -p "$BUILD_ROOT"

log()  { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[1;33m!!  %s\033[0m\n' "$*" >&2; }
fail() { printf '\033[1;31mxx  %s\033[0m\n' "$*" >&2; }

MISSING=()
note_missing() { MISSING+=("$1"); warn "missing: $1"; }

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        note_missing "command: $1 (package hint: ${2:-$1})"
        return 1
    fi
    return 0
}

require_pkgconfig() {
    if ! pkg-config --exists "$1" 2>/dev/null; then
        note_missing "pkg-config module: $1"
        return 1
    fi
    return 0
}

require_header() {
    if [[ ! -f "$1" ]]; then
        note_missing "header: $1"
        return 1
    fi
    return 0
}

apt_install() {
    if [[ "$CHECK_ONLY" -eq 1 ]]; then
        for p in "$@"; do
            dpkg -s "$p" >/dev/null 2>&1 || note_missing "apt package: $p"
        done
        return 0
    fi
    # try the whole batch first (fast path); if apt rejects it (e.g. one bad/renamed package
    # name), fall back to installing one at a time so a single typo doesn't sink everything else
    if ! sudo apt-get install -y "$@"; then
        warn "batch install failed, retrying package-by-package: $*"
        local failed=()
        for p in "$@"; do
            sudo apt-get install -y "$p" || failed+=("$p")
        done
        if [[ "${#failed[@]}" -gt 0 ]]; then
            for p in "${failed[@]}"; do note_missing "apt package: $p (install failed)"; done
        fi
    fi
}

# ---------------------------------------------------------------------------
# 1. base packages + Visual Studio remote toolchain requirements
# ---------------------------------------------------------------------------
# The Orange Pi image is a SERVER image with no desktop — SSH is the only way in, so verifying
# it works is step zero, not an afterthought.
install_base() {
    log "Base packages + Visual Studio remote toolchain requirements"
    if [[ "$CHECK_ONLY" -eq 0 ]]; then
        sudo apt-get update
    fi
    apt_install \
        build-essential gcc g++ gdb gdbserver make ninja-build cmake \
        openssh-server rsync zip unzip tar git curl pkg-config \
        swig nlohmann-json3-dev

    if [[ "$CHECK_ONLY" -eq 0 ]]; then
        sudo systemctl enable --now ssh || warn "could not enable sshd — is this a container without systemd?"
    fi
    require_cmd gcc
    require_cmd g++
    require_cmd gdbserver gdbserver
    require_cmd swig swig
    require_cmd rsync
}

# ---------------------------------------------------------------------------
# 2. OpenCV 5.0, built from source with opencv_contrib
# ---------------------------------------------------------------------------
# Deliberately NOT the distro package (24.04 ships 4.6, and it's the wrong major version
# anyway) — built from source into /usr/local so it matches on both the WSL dev box and the Pi.
# opencv_contrib supplies the ArUco module needed for ChArUco calibration boards later.
OPENCV_VERSION="5.0.0"

build_opencv() {
    if [[ "$SKIP_OPENCV" -eq 1 ]]; then
        log "Skipping OpenCV build (--skip-opencv)"
        return 0
    fi

    log "OpenCV ${OPENCV_VERSION} (from source, with opencv_contrib)"

    # OpenCV 5.0 renamed both its pkg-config module and its header install directory from
    # opencv4 to opencv5 (confirmed by actually building it: headers land under
    # /usr/local/include/opencv5, module is `opencv5`, NOT `opencv4`) - if this ever changes
    # again in a later 5.x release, this is the line to update, along with FRCVLib.vcxproj's
    # AdditionalIncludeDirectories.
    if pkg-config --exists opencv5 2>/dev/null; then
        local installed_ver
        installed_ver="$(pkg-config --modversion opencv5)"
        if [[ "$installed_ver" == "$OPENCV_VERSION"* ]]; then
            log "OpenCV ${installed_ver} already installed at the requested version, skipping build"
            return 0
        else
            warn "found OpenCV ${installed_ver} via pkg-config; expected ${OPENCV_VERSION}.x — rebuilding"
        fi
    fi

    if [[ "$CHECK_ONLY" -eq 1 ]]; then
        note_missing "OpenCV ${OPENCV_VERSION} (opencv5.pc not found or wrong version)"
        return 0
    fi

    # purge the distro's OpenCV -dev packages first: they install headers under /usr/include
    # rather than /usr/local/include, but leaving them in place is still a footgun — the vcxproj
    # puts /usr/local/include first specifically so a from-source install wins, but that ordering
    # is easy to regress, and apt could reinstall these as a dependency of something else later.
    # Purging removes the ambiguity entirely instead of relying on include-order discipline.
    if dpkg -s libopencv-core-dev >/dev/null 2>&1; then
        warn "purging distro OpenCV -dev packages to avoid a stale /usr/include/opencv4 shadowing this build"
        sudo apt-get purge -y 'libopencv-*-dev' 'libopencv-*t64' || true
    fi

    apt_install \
        libgtk-3-dev libavcodec-dev libavformat-dev libswscale-dev libv4l-dev \
        libtbb-dev libjpeg-dev libpng-dev libtiff-dev python3-dev python3-numpy

    local src="$BUILD_ROOT/opencv-${OPENCV_VERSION}"
    if [[ ! -d "$src" ]]; then
        git clone --branch "${OPENCV_VERSION}" --depth 1 https://github.com/opencv/opencv.git "$src"
        git clone --branch "${OPENCV_VERSION}" --depth 1 https://github.com/opencv/opencv_contrib.git "$src-contrib"
    fi

    mkdir -p "$src/build"
    (
        cd "$src/build"
        cmake -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$PREFIX" \
            -DOPENCV_EXTRA_MODULES_PATH="$src-contrib/modules" \
            -DBUILD_opencv_aruco=ON \
            -DBUILD_TESTS=OFF \
            -DBUILD_PERF_TESTS=OFF \
            -DBUILD_EXAMPLES=OFF \
            -DBUILD_opencv_python3=OFF \
            -DBUILD_opencv_java=OFF \
            -DBUILD_opencv_java_bindings_generator=OFF \
            -DOPENCV_GENERATE_PKGCONFIG=ON \
            -DWITH_FFMPEG=ON \
            ..
        cmake --build . --parallel "$JOBS"
        sudo cmake --install .
        sudo ldconfig
    )
    log "OpenCV ${OPENCV_VERSION} installed to ${PREFIX}"
}

# ---------------------------------------------------------------------------
# 3. AprilTag (AprilRobotics)
# ---------------------------------------------------------------------------
# Ubuntu 24.04's libapriltag-dev (3.3.0) does ship apriltag_pose.h — verified against a live
# 24.04 install, contrary to what older Ubuntu releases shipped. Prefer the apt package; only
# fall back to building from source if apriltag_pose.h turns out to be missing (e.g. an older
# base image, or a future package that drops it again).
# AprilTag: built from the SAME patched v3.4.5 source vkapriltag's own CMake fetches
# (cmake/patches/apriltag-expose-decode-steps.patch, applied against AprilRobotics/apriltag
# v3.4.5), installed as the system's only apriltag - not apt's package, and not a second,
# separately-built copy. This matters because both this build and vkapriltag's own FetchContent
# build produce a shared library with the SAME SONAME (libapriltag.so.3) regardless of the
# 3.3.0-vs-3.4.5 version difference: confirmed by actually building both and checking. Whichever
# libapriltag.so.3 the dynamic linker resolves at runtime is used by BOTH the CPU AprilTag
# backend and vkapriltag's VkApriltagBackend - and only the patched build exports the two
# symbols (quad_decode_index, reconcile_detections) VkApriltagBackend needs. So there must be
# exactly one apriltag in the system, and it must be this patched one; apt's package and a
# vanilla source build are both wrong for this project once FRCV_WITH_VULKAN_APRILTAG is in play.
APRILTAG_TAG="${APRILTAG_TAG:-v3.4.5}"

build_apriltag() {
    log "AprilTag ${APRILTAG_TAG} (patched for vkapriltag)"

    if require_header /usr/local/include/apriltag/apriltag_pose.h 2>/dev/null; then
        # a header check alone can't tell the patched build apart from a vanilla one; the
        # patch only adds new *symbols*, not new headers. If this is stale from a previous
        # run of the OLD (apt-based or vanilla-source) version of this script, the symbol
        # check in the vkapriltag build/link step later is what will actually catch it.
        log "apriltag already installed under /usr/local"
        return 0
    fi

    # purge only the packages that are actually installed - `apt-get purge` aborts the WHOLE
    # command over one unknown package name (confirmed: an earlier version of this listed
    # libapriltag-utils3t64, which doesn't exist, and that silently left both real packages
    # in place because of the trailing `|| true`)
    local installed_apriltag_pkgs=()
    for pkg in libapriltag-dev libapriltag3t64 libapriltag-utils3t64; do
        dpkg -s "$pkg" >/dev/null 2>&1 && installed_apriltag_pkgs+=("$pkg")
    done
    if [[ "${#installed_apriltag_pkgs[@]}" -gt 0 ]]; then
        warn "purging apt's apriltag package(s) [${installed_apriltag_pkgs[*]}] - it would collide (same SONAME, older/unpatched) with the patched build this project needs"
        sudo apt-get purge -y "${installed_apriltag_pkgs[@]}"
    fi

    if [[ "$CHECK_ONLY" -eq 1 ]]; then
        note_missing "AprilTag ${APRILTAG_TAG} (patched) not found under /usr/local/include"
        return 0
    fi

    local src="$BUILD_ROOT/apriltag-patched"
    local patch_file="$BUILD_ROOT/../third_party/vkapriltag/apriltags_vulkan/cmake/patches/apriltag-expose-decode-steps.patch"
    # resolve relative to this script's location too, in case BUILD_ROOT isn't under the repo
    if [[ ! -f "$patch_file" ]]; then
        patch_file="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/third_party/vkapriltag/apriltags_vulkan/cmake/patches/apriltag-expose-decode-steps.patch"
    fi
    if [[ ! -f "$patch_file" ]]; then
        fail "can't find vkapriltag's apriltag patch file - is the third_party/vkapriltag submodule checked out? (git submodule update --init)"
        return 1
    fi

    if [[ ! -d "$src" ]]; then
        git clone --branch "$APRILTAG_TAG" --depth 1 https://github.com/AprilRobotics/apriltag.git "$src"
    fi
    (
        cd "$src"
        # idempotent, matching vkapriltag's own PATCH_COMMAND: skip re-applying if already applied
        git apply --reverse --check "$patch_file" 2>/dev/null || git apply "$patch_file"
    )
    mkdir -p "$src/build"
    (
        cd "$src/build"
        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" ..
        cmake --build . --parallel "$JOBS"
        sudo cmake --install .
        sudo ldconfig
    )
}

# ---------------------------------------------------------------------------
# 4. FFmpeg dev headers (+ rkmpp hardware encoder on aarch64)
# ---------------------------------------------------------------------------
install_ffmpeg() {
    log "FFmpeg development headers"
    apt_install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

    if [[ "$ARCH" == "aarch64" ]]; then
        log "Checking for the rkmpp-enabled FFmpeg build (RK3588 hardware H.264 encode, phase 6)"
        # ubuntu-rockchip enables the rockchip-multimedia PPA by default, which carries an
        # rkmpp-patched ffmpeg — confirm it's actually present rather than assuming.
        if command -v ffmpeg >/dev/null 2>&1 && ffmpeg -hide_banner -encoders 2>/dev/null | grep -q h264_rkmpp; then
            log "h264_rkmpp encoder present"
        else
            note_missing "ffmpeg with h264_rkmpp encoder (check the rockchip-multimedia PPA is enabled: apt-cache policy ffmpeg)"
        fi
        for grp in video render; do
            if ! id -nG "$USER" | grep -qw "$grp"; then
                note_missing "user '$USER' in group '$grp' (needed for /dev/mpp_service access)"
                [[ "$CHECK_ONLY" -eq 0 ]] && sudo usermod -aG "$grp" "$USER" && \
                    warn "added $USER to $grp — log out/in (or reboot) for it to take effect"
            fi
        done
    fi
}

# ---------------------------------------------------------------------------
# 5. Vulkan (phase 5, vkapriltag) — on aarch64 this must resolve to the image's libmali blob
# ---------------------------------------------------------------------------
install_vulkan() {
    log "Vulkan development packages"
    apt_install libvulkan-dev vulkan-tools glslang-tools spirv-tools

    if [[ "$ARCH" != "aarch64" ]]; then
        return 0
    fi

    log "Checking Vulkan ICD (expecting the image's bundled libmali, not panfrost/panvk)"

    # ubuntu-rockchip installs several libmali*.so variants (x11, wayland-gbm, with/without
    # vulkan) but — as of this image — registers no /usr/share/vulkan/icd.d/*.json for any of
    # them, so nothing picks a driver until we write one. On a headless SERVER image the only
    # variant that can plausibly init without a display server is the "wayland-gbm" one (GBM
    # talks to the kernel DRM/GBM API directly, no compositor needed) — and of those, only the
    # one with "-vulkan" in its name actually implements the Vulkan ICD entry points; the
    # plain "-wayland-gbm" ones are OpenGL/EGL only. If the image's package version numbering
    # ever changes this filename, this glob still finds it by content, not a hardcoded name.
    local mali_lib
    mali_lib="$(find /usr/lib/aarch64-linux-gnu -maxdepth 1 -iname 'libmali-*wayland-gbm*vulkan*.so' 2>/dev/null | head -1)"

    local icd_dir=/usr/share/vulkan/icd.d
    local icd_json="$icd_dir/libmali-gbm.json"

    if [[ -d "$icd_dir" ]] && ls "$icd_dir"/*.json >/dev/null 2>&1; then
        log "Vulkan ICD(s) already registered:"
        ls "$icd_dir"/*.json
    elif [[ -n "$mali_lib" ]]; then
        if [[ "$CHECK_ONLY" -eq 1 ]]; then
            note_missing "Vulkan ICD not registered (would write $icd_json -> $mali_lib)"
        else
            log "No Vulkan ICD registered yet; writing one for $mali_lib"
            sudo mkdir -p "$icd_dir"
            sudo tee "$icd_json" >/dev/null <<EOF
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "$mali_lib",
        "api_version": "1.2.0"
    }
}
EOF
        fi
    else
        note_missing "libmali wayland-gbm+vulkan .so not found under /usr/lib/aarch64-linux-gnu — is this actually the ubuntu-rockchip image with libmali installed?"
    fi

    if command -v vulkaninfo >/dev/null 2>&1 && [[ "$CHECK_ONLY" -eq 0 ]]; then
        log "Running vulkaninfo --summary (expect a Mali device with a VK_QUEUE_COMPUTE_BIT queue family)"
        VK_ICD_FILENAMES="$icd_json" vulkaninfo --summary 2>&1 | tee "$BUILD_ROOT/vulkaninfo-summary.log" || \
            warn "vulkaninfo failed even headless with the gbm+vulkan ICD — see phase 5 item 0 in IMPLEMENTATION_PLAN.md; the CPU AprilTag backend remains the fallback"
    elif ! command -v vulkaninfo >/dev/null 2>&1; then
        note_missing "vulkaninfo (from vulkan-tools)"
    fi

    warn "if a panvk/lavapipe ICD later appears (e.g. from a mesa update), pin VK_ICD_FILENAMES=$icd_json in frcv.service so the wrong driver is never silently selected (see phase 9)"
}

# ---------------------------------------------------------------------------
# 5b. vkapriltag (phase 5) — builds the third_party/vkapriltag submodule's static library.
# ---------------------------------------------------------------------------
# vkapriltag statically links its own patched AprilRobotics/apriltag v3.4.5 fetch, which
# produces a shared library with the SAME SONAME (libapriltag.so.3) as any other apriltag
# build - confirmed by actually building both. build_apriltag() above installs exactly that
# patched build as the system's only /usr/local apriltag for this reason, so this function
# builds vkapriltag itself, letting FetchContent grab its own private copy for the build only
# (that private copy is never installed or linked into FRCVLib - only libvkapriltag.a is).
build_vkapriltag() {
    log "vkapriltag (Vulkan AprilTag detection submodule)"

    local repo_root
    repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
    local src="$repo_root/third_party/vkapriltag/apriltags_vulkan"
    local build_dir="$src/build"
    local lib_path="$build_dir/library/libvkapriltag.a"

    if [[ ! -d "$src" ]]; then
        fail "third_party/vkapriltag submodule not found - run: git submodule update --init"
        return 1
    fi

    if [[ -f "$lib_path" ]]; then
        log "libvkapriltag.a already built"
        return 0
    fi
    if [[ "$CHECK_ONLY" -eq 1 ]]; then
        note_missing "libvkapriltag.a not built yet ($lib_path)"
        return 0
    fi

    mkdir -p "$build_dir"
    (
        cd "$build_dir"
        # -fPIC: vkapriltag's own CMakeLists doesn't set POSITION_INDEPENDENT_CODE, but
        # libvkapriltag.a must go into FRCVLib's shared library - confirmed the hard way
        # (`recompile with -fPIC` at final link time) before adding this.
        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
            -DVKAPRILTAG_BUILD_APPS=OFF -DVKAPRILTAG_BUILD_TOOLS=OFF ..
        cmake --build . --target vkapriltag --parallel "$JOBS"
    )

    if [[ ! -f "$lib_path" ]]; then
        fail "vkapriltag build finished but $lib_path is missing - something changed in its CMakeLists"
        return 1
    fi
}

# ---------------------------------------------------------------------------
# 6. WebRTC (phase 6) — libdatachannel, opt-in via --with-webrtc
# ---------------------------------------------------------------------------
build_webrtc() {
    [[ "$WITH_WEBRTC" -eq 1 ]] || return 0
    log "libdatachannel (WebRTC)"

    apt_install libssl-dev libsrtp2-dev

    if require_pkgconfig libdatachannel 2>/dev/null; then
        log "libdatachannel already installed"
        return 0
    fi
    if [[ "$CHECK_ONLY" -eq 1 ]]; then
        note_missing "libdatachannel"
        return 0
    fi

    local src="$BUILD_ROOT/libdatachannel"
    if [[ ! -d "$src" ]]; then
        git clone --recursive --depth 1 https://github.com/paullouisageneau/libdatachannel.git "$src"
    fi
    mkdir -p "$src/build"
    (
        cd "$src/build"
        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
            -DUSE_GNUTLS=OFF -DUSE_NICE=OFF ..
        cmake --build . --parallel "$JOBS"
        sudo cmake --install .
        sudo ldconfig
    )
}

# ---------------------------------------------------------------------------
# 7. NT4 (phase 3) — ntcore + wpiutil, opt-in via --with-nt4
# ---------------------------------------------------------------------------
# WPILib publishes prebuilt C++ artifacts to Maven; there's no apt package. Classifier naming
# below is best-effort against current WPILib conventions — verify against
# https://frcmaven.wpi.edu/ui/repos/tree/General/release/edu/wpi/first/ntcore/ntcore-cpp before
# relying on it; the maven-metadata.xml under that path lists the actual current version.
NTCORE_VERSION="${NTCORE_VERSION:-2025.3.2}"

fetch_ntcore() {
    [[ "$WITH_NT4" -eq 1 ]] || return 0
    log "ntcore + wpiutil ${NTCORE_VERSION}"

    if require_header /usr/local/include/ntcore_cpp.h 2>/dev/null; then
        log "ntcore already installed"
        return 0
    fi
    if [[ "$CHECK_ONLY" -eq 1 ]]; then
        note_missing "ntcore (ntcore_cpp.h not found)"
        return 0
    fi

    local classifier
    case "$ARCH" in
        x86_64) classifier="linuxx86-64" ;;
        aarch64) classifier="linuxarm64" ;;
        *) fail "unsupported arch for ntcore: $ARCH"; return 1 ;;
    esac

    local base="https://frcmaven.wpi.edu/artifactory/release/edu/wpi/first"
    local dest="$BUILD_ROOT/ntcore"
    mkdir -p "$dest"
    # ntcore links against wpinet (confirmed via ldd - it's not just a wpiutil/ntcore pair)
    for artifact in ntcore/ntcore-cpp wpinet/wpinet-cpp wpiutil/wpiutil-cpp; do
        local name="${artifact#*/}"
        for kind in headers "${classifier}"; do
            local url="${base}/${artifact}/${NTCORE_VERSION}/${name}-${NTCORE_VERSION}-${kind}.zip"
            curl -fsSL "$url" -o "$dest/${name}-${kind}.zip" || {
                warn "download failed: $url — check the version/classifier against frcmaven.wpi.edu"
                continue
            }
            unzip -oq "$dest/${name}-${kind}.zip" -d "$dest/${name}-${kind}"
        done
    done
    sudo cp -r "$dest"/*headers*/* "$PREFIX/include/" 2>/dev/null || true
    # the shared libraries are nested (e.g. linux/x86-64/shared/libntcore.so), not at the zip
    # root, so a shallow glob here finds nothing - search recursively instead
    find "$dest" -path "*${classifier}*" \( -name '*.so' -o -name '*.so.*' \) -print0 | \
        xargs -0 -r sudo cp -t "$PREFIX/lib/"
    sudo ldconfig
}

# ---------------------------------------------------------------------------
# 8. ONNX Runtime — official prebuilt release, CPU execution provider only
# ---------------------------------------------------------------------------
ORT_VERSION="${ORT_VERSION:-1.20.0}"

fetch_onnxruntime() {
    log "ONNX Runtime ${ORT_VERSION} (official prebuilt, CPU EP)"

    if require_header /usr/local/include/onnxruntime_cxx_api.h 2>/dev/null; then
        log "onnxruntime already installed"
        return 0
    fi
    if [[ "$CHECK_ONLY" -eq 1 ]]; then
        note_missing "onnxruntime (onnxruntime_cxx_api.h not found)"
        return 0
    fi

    local ort_arch
    case "$ARCH" in
        x86_64) ort_arch="x64" ;;
        aarch64) ort_arch="aarch64" ;;
        *) fail "unsupported arch for onnxruntime: $ARCH"; return 1 ;;
    esac

    local tarname="onnxruntime-linux-${ort_arch}-${ORT_VERSION}"
    local url="https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/${tarname}.tgz"
    local dest="$BUILD_ROOT/onnxruntime"
    mkdir -p "$dest"
    curl -fsSL "$url" -o "$dest/$tarname.tgz"
    tar -xzf "$dest/$tarname.tgz" -C "$dest"
    sudo cp -r "$dest/$tarname/include/"* "$PREFIX/include/"
    sudo cp -r "$dest/$tarname/lib/"* "$PREFIX/lib/"
    sudo ldconfig
}

# ---------------------------------------------------------------------------
# 9. aarch64-only: RKNN runtime (RK3588 NPU)
# ---------------------------------------------------------------------------
# librknnrt.so must match the kernel's rknpu driver version or rknn_init() fails with an opaque
# error — check both explicitly rather than assuming a fresh checkout is compatible.
RKNN_TOOLKIT2_REF="${RKNN_TOOLKIT2_REF:-master}"

fetch_rknn() {
    [[ "$ARCH" == "aarch64" ]] || return 0
    log "RKNN runtime (rknn-toolkit2, ref=${RKNN_TOOLKIT2_REF})"

    local driver_version=""
    if [[ -r /sys/kernel/debug/rknpu/version ]]; then
        driver_version="$(sudo cat /sys/kernel/debug/rknpu/version 2>/dev/null || true)"
    fi
    if [[ -z "$driver_version" ]]; then
        driver_version="$(dmesg 2>/dev/null | grep -i rknpu | grep -oP 'version:\s*\K[0-9.]+' | tail -1 || true)"
    fi
    [[ -n "$driver_version" ]] && log "kernel rknpu driver version: $driver_version" \
        || warn "could not determine kernel rknpu driver version — check /sys/kernel/debug/rknpu/version manually and compare against the librknnrt.so you install"

    if require_header /usr/include/rknn_api.h 2>/dev/null; then
        log "rknn_api.h already installed"
        return 0
    fi
    if [[ "$CHECK_ONLY" -eq 1 ]]; then
        note_missing "RKNN runtime (rknn_api.h not found)"
        return 0
    fi

    local src="$BUILD_ROOT/rknn-toolkit2"
    if [[ ! -d "$src" ]]; then
        git clone --branch "$RKNN_TOOLKIT2_REF" --depth 1 https://github.com/airockchip/rknn-toolkit2.git "$src"
    fi
    local rt_dir="$src/rknpu2/runtime/Linux/librknn_api"
    if [[ ! -d "$rt_dir" ]]; then
        fail "expected runtime layout not found at $rt_dir — rknn-toolkit2's repo layout may have changed; locate librknnrt.so and rknn_api.h manually"
        return 1
    fi
    sudo cp "$rt_dir/include/rknn_api.h" "$PREFIX/include/"
    sudo cp "$rt_dir/aarch64/librknnrt.so" "$PREFIX/lib/"
    sudo ldconfig
    warn "verify the copied librknnrt.so version matches the kernel driver reported above"
}

# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
log "FRCV dependency check/install — arch=$ARCH, check-only=$CHECK_ONLY, jobs=$JOBS"

install_base
build_opencv
build_apriltag
install_ffmpeg
install_vulkan
build_vkapriltag || warn "vkapriltag build failed - see the log above; the CPU AprilTag backend remains the fallback"
# these three are optional/best-effort integrations (WebRTC, NT4, RKNN) - a failure partway
# through one of them (a bad ref, a flaky download) should not, under `set -e`, take down a
# run that otherwise succeeded; ONNX Runtime stays unconditional since --with-* doesn't gate it
build_webrtc || warn "WebRTC setup (libdatachannel) failed - see the log above; continuing"
fetch_ntcore || warn "NT4 setup (ntcore/wpiutil/wpinet) failed - see the log above; continuing"
fetch_onnxruntime
fetch_rknn || warn "RKNN setup failed - see the log above; continuing"

if [[ "$CHECK_ONLY" -eq 1 ]]; then
    echo
    if [[ "${#MISSING[@]}" -eq 0 ]]; then
        log "All checked dependencies are present."
    else
        fail "${#MISSING[@]} dependency issue(s) found:"
        for m in "${MISSING[@]}"; do echo "  - $m"; done
        exit 1
    fi
else
    log "Done. Re-run with --check at any time to verify the environment."
fi
