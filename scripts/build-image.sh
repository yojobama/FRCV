#!/usr/bin/env bash
# Builds a customized, flashable Orange Pi 5-family OS image: downloads Armbian's own published
# Debian 13 "trixie" vendor-kernel image for the given board, installs the proprietary libmali
# (Mali G610, driver release "g24p0") Vulkan/GLES userspace driver and the already-built
# lumenvision-backend .deb into it via systemd-nspawn, sets the hostname, and re-compresses the
# result.
#
# Why customize Armbian's own image instead of building Armbian from source: Armbian already
# publishes exactly this board-support/driver work (kernel, u-boot, DTBs) per board, continuously
# - reimplementing that here would just mean redoing what Armbian's own maintainers already do.
# This script only adds what Armbian's image doesn't have: this project's own software, and the
# one proprietary driver package no apt repository currently mirrors as a binary (see the libmali
# section below).
#
# Why systemd-nspawn, not a plain chroot: scripts/deb/postinst calls `systemctl daemon-reload`
# (unguarded, under `set -e`) plus `systemctl enable`/`restart` - none of which work without a
# running systemd/D-Bus, which a bare chroot doesn't have. `systemd-nspawn -b` boots a real (if
# lightweight) systemd inside the container, so postinst runs completely unmodified - the exact
# same script that already works on real hardware.
#
# Why libmali is built from source here rather than downloaded: JeffyCN/mirrors (the
# actively-maintained fork of rockchip-linux/libmali) ships no GitHub Releases - its `libmali`
# branch is a Debian/meson SOURCE tree only (confirmed directly against the GitHub API, not
# assumed). Building it via dpkg-buildpackage matches this project's own established pattern
# (install-deps.sh already builds MPP/ffmpeg-rockchip/apriltag/vkapriltag from source rather than
# trusting a third-party binary mirror). The proprietary blob is required, not Panthor/Mesa (the
# open-source alternative some newer Armbian RK3588 builds default to) - measured directly at
# ~14x slower than libmali for this project's own vkapriltag Vulkan compute workload.
#
# Usage: VERSION=1.2.3 scripts/build-image.sh <board-slug>
#   board-slug - one of: orangepi5 orangepi5-plus orangepi5b orangepi5pro orangepi5-max
#                orangepi5-ultra
#   VERSION    - same package version build-deb.sh was given - required.
#   DEB_FILE   - path to the already-built lumenvision-backend_*_arm64.deb - optional, defaults
#                to the single .deb found in the repo root (what build-deb.sh just produced, the
#                same convention .github/workflows/release.yml's build-images job relies on).
# Produces: lumenvision-<slug>_<VERSION>.img.xz in the repo root.
#
# Must run as root (loop devices, mount, systemd-nspawn) on a genuine aarch64 Linux machine (an
# `ubuntu-24.04-arm` CI runner, or real ARM hardware) - both the base image and the libmali build
# are aarch64, and systemd-nspawn needs a matching host to execute them natively.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

if [[ "$(uname -m)" != "aarch64" ]]; then
    echo "build-image.sh must run on aarch64 (an arm64 CI runner, or real ARM hardware) - got $(uname -m)" >&2
    exit 1
fi
if [[ "$EUID" -ne 0 ]]; then
    echo "build-image.sh must run as root (loop devices, mount, systemd-nspawn)" >&2
    exit 1
fi

SLUG="${1:?usage: scripts/build-image.sh <board-slug>}"
VERSION="${VERSION:?set VERSION=x.y.z (no leading v) before running this script}"

case "$SLUG" in
    orangepi5|orangepi5-plus|orangepi5b|orangepi5pro|orangepi5-max|orangepi5-ultra) ;;
    *)
        echo "unknown board slug '$SLUG' - expected one of: orangepi5 orangepi5-plus orangepi5b orangepi5pro orangepi5-max orangepi5-ultra" >&2
        exit 1
        ;;
esac

DEB_FILE="${DEB_FILE:-}"
if [[ -z "$DEB_FILE" ]]; then
    mapfile -t debs < <(find "$REPO_ROOT" -maxdepth 1 -name 'lumenvision-backend_*_arm64.deb')
    if [[ "${#debs[@]}" -ne 1 ]]; then
        echo "expected exactly one lumenvision-backend_*_arm64.deb in $REPO_ROOT (found ${#debs[@]}) - build it first (scripts/build-deb.sh) or set DEB_FILE explicitly" >&2
        exit 1
    fi
    DEB_FILE="${debs[0]}"
fi
DEB_FILE="$(cd "$(dirname "$DEB_FILE")" && pwd)/$(basename "$DEB_FILE")"

LIBMALI_PKG="libmali-valhall-g610-g24p0-gbm"
WORK_DIR="$REPO_ROOT/out/image-${SLUG}"
BASE_IMG_XZ="$WORK_DIR/base.img.xz"
BASE_IMG="$WORK_DIR/base.img"
MOUNT_DIR="$WORK_DIR/rootfs"
LIBMALI_SRC="$WORK_DIR/libmali-src"
OUT_IMG="$REPO_ROOT/lumenvision-${SLUG}_${VERSION}.img.xz"
LOOP_DEV=""
CONTAINER_NAME="lumen-img-${SLUG}"
CONTAINER_BOOTED=0

cleanup() {
    set +e
    if [[ "$CONTAINER_BOOTED" -eq 1 ]]; then
        machinectl poweroff "$CONTAINER_NAME" >/dev/null 2>&1
        for _ in $(seq 1 30); do
            machinectl show "$CONTAINER_NAME" >/dev/null 2>&1 || break
            sleep 1
        done
    fi
    if [[ -n "$LOOP_DEV" ]]; then
        umount "$MOUNT_DIR" >/dev/null 2>&1
        losetup -d "$LOOP_DEV" >/dev/null 2>&1
    fi
}
trap cleanup EXIT

rm -rf "$WORK_DIR" "$OUT_IMG"
mkdir -p "$WORK_DIR" "$MOUNT_DIR"

echo "==> [1/6] Downloading Armbian base image for $SLUG"
curl -fL "https://dl.armbian.com/${SLUG}/Trixie_vendor_minimal" -o "$BASE_IMG_XZ"
curl -fL "https://dl.armbian.com/${SLUG}/Trixie_vendor_minimal.sha" -o "$BASE_IMG_XZ.sha"

echo "==> Verifying checksum"
EXPECTED_SHA="$(awk '{print $1}' "$BASE_IMG_XZ.sha")"
ACTUAL_SHA="$(sha256sum "$BASE_IMG_XZ" | awk '{print $1}')"
if [[ -z "$EXPECTED_SHA" || "$EXPECTED_SHA" != "$ACTUAL_SHA" ]]; then
    echo "checksum mismatch for $SLUG base image: expected '$EXPECTED_SHA', got '$ACTUAL_SHA'" >&2
    exit 1
fi

echo "==> [2/6] Building $LIBMALI_PKG from source (JeffyCN/mirrors, libmali branch)"
apt-get update
apt-get install -y --no-install-recommends \
    build-essential dpkg-dev debhelper meson ninja-build pkg-config git \
    libgbm-dev libdrm-dev libx11-xcb1 libxcb-dri2-0 libxdamage1 libxext6 libwayland-client0 \
    systemd-container xz-utils cloud-guest-utils e2fsprogs

git clone --depth 1 --branch libmali https://github.com/JeffyCN/mirrors.git "$LIBMALI_SRC"

# Trim debian/targets and debian/control down to just the one G610 "g24" plain-gbm (headless, no
# X11/Wayland compositor needed on a server board) variant - the untrimmed source tree builds
# ~25 unrelated GPU-family/platform combinations (bifrost/midgard/utgard, other Valhall GPUs,
# X11/Wayland variants) via debian/rules' own `for target in $(TARGETS)` loop, none of which this
# project will ever install.
grep -F "${LIBMALI_PKG}.so" "$LIBMALI_SRC/debian/targets" > "$LIBMALI_SRC/debian/targets.filtered"
mv "$LIBMALI_SRC/debian/targets.filtered" "$LIBMALI_SRC/debian/targets"
awk -v RS="" -v pkg="Package: ${LIBMALI_PKG}" '
    {
        split($0, lines, "\n")
        if (NR == 1 || lines[1] == pkg) print $0 "\n"
    }
' "$LIBMALI_SRC/debian/control" > "$LIBMALI_SRC/debian/control.filtered"
mv "$LIBMALI_SRC/debian/control.filtered" "$LIBMALI_SRC/debian/control"

(cd "$LIBMALI_SRC" && dpkg-buildpackage -b -uc -us)

mapfile -t libmali_debs < <(find "$WORK_DIR" -maxdepth 1 -name "${LIBMALI_PKG}_*.deb")
if [[ "${#libmali_debs[@]}" -ne 1 ]]; then
    echo "expected exactly one ${LIBMALI_PKG}_*.deb after dpkg-buildpackage (found ${#libmali_debs[@]})" >&2
    exit 1
fi
LIBMALI_DEB="${libmali_debs[0]}"
echo "==> Built $LIBMALI_DEB"

echo "==> [3/6] Extracting base image and mapping partitions"
unxz -k "$BASE_IMG_XZ"

# Armbian's published image ships with its root partition sized tight (just enough for the base
# OS) - it only grows to fill the real SD card/eMMC via a first-boot resize service, which never
# runs here since this script only ever mounts the raw .img through a loop device, it never
# actually boots the image on real hardware. Confirmed the hard way: installing libmali +
# lumenvision-backend (which bundles OpenCV/ONNX Runtime/ffmpeg-rockchip/etc. - "every
# from-source runtime dependency", per its own .deb Description) into the un-grown image failed
# outright with "No space left on device" partway through unpacking. Growing by a fixed, generous
# 3GB up front is simpler and safer than precisely sizing to the .deb's own Installed-Size - the
# extra headroom costs almost nothing in the final .img.xz (xz compresses empty ext4 blocks to
# near nothing) and the image gets shrunk to fit the real card/eMMC again on first real boot
# anyway (Armbian's own resize service works in the other direction too).
echo "==> Growing image +3GB to fit the customization (Armbian ships its base image sized tight)"
truncate -s +3G "$BASE_IMG"
LOOP_DEV="$(losetup --find --show -P "$BASE_IMG")"
sleep 1
growpart "$LOOP_DEV" 1
e2fsck -f -y "${LOOP_DEV}p1" || true
resize2fs "${LOOP_DEV}p1"

# Armbian images: partition 1 is the single ext4 root (u-boot lives in raw sectors before
# partition 1, not a separate partition, so there's nothing else here to mount).
mount "${LOOP_DEV}p1" "$MOUNT_DIR"

echo "==> [4/6] Staging the .deb files into the container"
cp "$DEB_FILE" "$MOUNT_DIR/root/$(basename "$DEB_FILE")"
cp "$LIBMALI_DEB" "$MOUNT_DIR/root/$(basename "$LIBMALI_DEB")"
CUSTOMIZE_SCRIPT="$MOUNT_DIR/root/customize.sh"
cat > "$CUSTOMIZE_SCRIPT" <<EOS
#!/bin/bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

echo "----> apt-get update"
apt-get update

echo "----> Installing libmali (proprietary Mali G610 driver - NOT Panthor/Mesa)"
apt-get install -y /root/${LIBMALI_PKG}_*.deb

echo "----> Clearing any pre-existing Vulkan ICD registration (e.g. a base-image Panthor/Mesa one)"
mkdir -p /usr/share/vulkan/icd.d
find /usr/share/vulkan/icd.d -maxdepth 1 -iname '*.json' -delete

MALI_SO="\$(dpkg -L ${LIBMALI_PKG} | grep -iE '\.so(\.[0-9]+)*\$' | grep -i vulkan | head -1)"
if [[ -z "\$MALI_SO" ]]; then
    MALI_SO="\$(dpkg -L ${LIBMALI_PKG} | grep -iE '\.so(\.[0-9]+)*\$' | head -1)"
fi
if [[ -z "\$MALI_SO" ]]; then
    echo "${LIBMALI_PKG} installed no .so file - packaging must have changed, cannot register a Vulkan ICD" >&2
    exit 1
fi
echo "----> Registering Vulkan ICD: \$MALI_SO"
cat > /usr/share/vulkan/icd.d/libmali.json <<EOF
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "\$MALI_SO",
        "api_version": "1.2.0"
    }
}
EOF

mkdir -p /etc/systemd/system/lumenvision.service.d
cat > /etc/systemd/system/lumenvision.service.d/10-vulkan-icd.conf <<EOF
# Written by scripts/build-image.sh at image-build time - pins the board's Vulkan ICD to the
# proprietary libmali driver installed above, not whatever the base Armbian image would
# otherwise default to (Panthor/Mesa, measured ~14x slower for vkapriltag's compute workload).
[Service]
Environment=VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/libmali.json
EOF

echo "----> Installing the lumenvision-backend .deb"
apt-get install -y /root/lumenvision-backend_*_arm64.deb

echo "----> Setting hostname to lumenvision"
OLD_HOSTNAME="\$(cat /etc/hostname 2>/dev/null || true)"
echo lumenvision > /etc/hostname
if [[ -n "\$OLD_HOSTNAME" ]] && grep -q "\$OLD_HOSTNAME" /etc/hosts 2>/dev/null; then
    sed -i "s/\b\$OLD_HOSTNAME\b/lumenvision/g" /etc/hosts
fi
if ! grep -q '^127\.0\.1\.1' /etc/hosts 2>/dev/null; then
    echo -e "127.0.1.1\tlumenvision" >> /etc/hosts
fi
hostnamectl set-hostname lumenvision || true

echo "----> Bundled librknnrt.so version (for the record - full RKNPU compatibility can only be confirmed by booting real hardware)"
RKNN_SO="\$(find /opt/lumenvision -maxdepth 1 -iname 'librknnrt.so' 2>/dev/null | head -1)"
if [[ -n "\$RKNN_SO" ]]; then
    strings "\$RKNN_SO" | grep -m1 -i 'librknnrt version' || echo "librknnrt.so present but no version string found in \$RKNN_SO"
else
    echo "librknnrt.so not found under /opt/lumenvision - this .deb may have been built without --with-rknn"
fi

echo "----> Cleaning up"
apt-get clean
rm -rf /var/lib/apt/lists/*
rm -f /root/*.deb /root/customize.sh
EOS
chmod +x "$CUSTOMIZE_SCRIPT"

echo "==> [5/6] Booting systemd-nspawn and running the customization"
systemd-nspawn -D "$MOUNT_DIR" -b --machine="$CONTAINER_NAME" --resolv-conf=copy-host >/dev/null 2>&1 &
CONTAINER_BOOTED=1

READY=0
for _ in $(seq 1 60); do
    if systemd-run --machine="$CONTAINER_NAME" --quiet --wait --pipe -- true >/dev/null 2>&1; then
        READY=1
        break
    fi
    sleep 1
done
if [[ "$READY" -ne 1 ]]; then
    echo "container $CONTAINER_NAME never became ready (systemd inside it didn't finish booting)" >&2
    exit 1
fi

systemd-run --machine="$CONTAINER_NAME" --quiet --wait --pipe -- /bin/bash /root/customize.sh

machinectl poweroff "$CONTAINER_NAME" >/dev/null 2>&1
for _ in $(seq 1 30); do
    machinectl show "$CONTAINER_NAME" >/dev/null 2>&1 || break
    sleep 1
done
CONTAINER_BOOTED=0

echo "==> [6/6] Unmounting and re-compressing"
umount "$MOUNT_DIR"
losetup -d "$LOOP_DEV"
LOOP_DEV=""

xz -T0 -c "$BASE_IMG" > "$OUT_IMG"
echo "==> Done: $OUT_IMG"
