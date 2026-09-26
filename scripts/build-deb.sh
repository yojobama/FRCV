#!/usr/bin/env bash
# Builds a single, self-contained .deb (lumenvision-backend) from the current source tree, for
# whichever architecture this script runs on - arm64 (a real Orange Pi, or an `ubuntu-24.04-arm`
# CI runner) or amd64 (a plain x64 Linux box - dev machine, CI sanity build, a non-Pi
# coprocessor). Must run on a machine that has already run scripts/install-deps.sh:
# LumenCore's own Linux POST_BUILD step (cmake/CopyLinuxRuntimeDeps.cmake) bundles every
# from-source runtime dependency - OpenCV, ffmpeg-rockchip, librga, etc. - into the package, so
# they must genuinely exist under /usr/local and /opt/lumenvision-ffmpeg on THIS machine at build
# time, not just be declared. (On amd64, install-deps.sh is run without any of the Rockchip-only
# flags - --with-rknn/--with-mpp/--with-ffmpeg-rockchip - since none of that exists off aarch64;
# the resulting package simply has fewer features, exactly like the ci-linux-x64 CI job already
# builds and tests.)
#
# Usage: VERSION=1.2.3 scripts/build-deb.sh
#   VERSION           - package version (Debian policy: digits/dots, no leading "v") - required.
#   LUMEN_CORE_PRESET  - CMake preset to configure/build LumenCore with - optional, defaults to
#                        pi-arm64-release on aarch64 (the right choice on a real Orange Pi) or
#                        ci-linux-x64 on x86_64 (the only preset that makes sense there without
#                        real Rockchip hardware). CI overrides this explicitly either way
#                        (ci-linux-arm64 / ci-linux-x64 - see .github/workflows/release.yml) so
#                        the native build stays under the SAME explicit-every-flag preset its own
#                        earlier configure/build/test steps already used, rather than this
#                        script silently re-resolving a different, unconfigured one.
# Produces: lumenvision-backend_<VERSION>_<arm64|amd64>.deb in the repo root.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

VERSION="${VERSION:?set VERSION=x.y.z (no leading v) before running this script}"
PKG_NAME="lumenvision-backend"
STAGE_DIR="$REPO_ROOT/out/deb-stage"

# Debian arch name + dotnet RID, derived once from the host - every other arch-specific spot in
# this script uses these two, not a literal "arm64"/"linux-arm64".
HOST_ARCH="$(uname -m)"
case "$HOST_ARCH" in
    aarch64)
        DEB_ARCH="arm64"
        DOTNET_RID="linux-arm64"
        LUMEN_CORE_PRESET="${LUMEN_CORE_PRESET:-pi-arm64-release}"
        ;;
    x86_64)
        DEB_ARCH="amd64"
        DOTNET_RID="linux-x64"
        LUMEN_CORE_PRESET="${LUMEN_CORE_PRESET:-ci-linux-x64}"
        ;;
    *)
        echo "build-deb.sh only knows how to package aarch64 or x86_64 - got $HOST_ARCH" >&2
        exit 1
        ;;
esac
DEB_FILE="$REPO_ROOT/${PKG_NAME}_${VERSION}_${DEB_ARCH}.deb"

echo "==> Cleaning previous stage"
rm -rf "$STAGE_DIR" "$DEB_FILE"
mkdir -p "$STAGE_DIR/DEBIAN" "$STAGE_DIR/opt/lumenvision" \
         "$STAGE_DIR/etc/systemd/system" "$STAGE_DIR/etc/udev/rules.d"

echo "==> Building webui"
(cd webui && npm ci && npm run build)

# Configure/build LumenCore explicitly rather than relying on dotnet publish's own BuildLumenCore
# MSBuild hook alone - that hook only re-BUILDS an already-configured preset (it checks for an
# existing CMakeCache.txt and warns+skips otherwise, see Server.csproj's own comment), it never
# runs the initial `cmake --preset` configure step. A from-scratch machine (a fresh CI runner, or
# a Pi that's never been configured for this preset before) needs that done explicitly first.
# Idempotent - a no-op on a machine that already configured/built this preset itself.
echo "==> Configuring/building LumenCore ($LUMEN_CORE_PRESET)"
cmake --preset "$LUMEN_CORE_PRESET"
cmake --build --preset "$LUMEN_CORE_PRESET"

echo "==> Publishing Server (self-contained, $DOTNET_RID, Release)"
dotnet publish "$REPO_ROOT/Server/Server.csproj" -c Release -r "$DOTNET_RID" --self-contained true \
    -p:LumenCorePreset="$LUMEN_CORE_PRESET"

PUBLISH_DIR="$REPO_ROOT/Server/bin/Release/net10.0/$DOTNET_RID/publish"
if [[ ! -f "$PUBLISH_DIR/Server" ]]; then
    echo "publish output not found at $PUBLISH_DIR - dotnet publish must have failed" >&2
    exit 1
fi

echo "==> Assembling package tree"
cp -r "$PUBLISH_DIR/." "$STAGE_DIR/opt/lumenvision/"
mkdir -p "$STAGE_DIR/opt/lumenvision/wwwroot"
cp -r webui/dist/. "$STAGE_DIR/opt/lumenvision/wwwroot/"
chmod +x "$STAGE_DIR/opt/lumenvision/Server"

cp scripts/lumenvision.service "$STAGE_DIR/etc/systemd/system/lumenvision.service"

# mirrors install-deps.sh --with-mpp's own rule (fetch_mpp's own comment there has the full
# story) - written here too since a .deb install must not depend on install-deps.sh having ever
# run on the target machine for anything beyond the build-time dependency compile this script
# itself already requires.
cat > "$STAGE_DIR/etc/udev/rules.d/99-lumenvision-rockchip.rules" <<'EOF'
# Installed by the lumenvision-backend .deb. Without this, these device nodes ship root-only
# (crw-------), and MPP/RGA hardware init fails with an opaque error rather than a permissions
# one.
KERNEL=="mpp_service", GROUP="video", MODE="0660"
SUBSYSTEM=="dma_heap", GROUP="video", MODE="0660"
KERNEL=="rga", GROUP="video", MODE="0660"
EOF

echo "==> Writing DEBIAN control files"
INSTALLED_SIZE_KB=$(du -sk "$STAGE_DIR/opt" "$STAGE_DIR/etc" | awk '{sum+=$1} END {print sum}')
# falls back to a fixed, non-empty identity rather than a blank/malformed "Name <email>" when
# git config isn't set (the common case on a CI runner or a bare source checkout, not just a
# hypothetical) - confirmed the hard way (an unset git user.name left a leading space before the
# email on a real build).
GIT_MAINTAINER_NAME="$(git config user.name 2>/dev/null || true)"
GIT_MAINTAINER_EMAIL="$(git config user.email 2>/dev/null || true)"
if [[ -n "$GIT_MAINTAINER_NAME" && -n "$GIT_MAINTAINER_EMAIL" ]]; then
    MAINTAINER="$GIT_MAINTAINER_NAME <$GIT_MAINTAINER_EMAIL>"
else
    MAINTAINER="LumenVision <noreply@example.invalid>"
fi

cat > "$STAGE_DIR/DEBIAN/control" <<EOF
Package: $PKG_NAME
Version: $VERSION
Section: misc
Priority: optional
Architecture: $DEB_ARCH
Installed-Size: $INSTALLED_SIZE_KB
Depends: avahi-daemon, libdrm2, libvulkan1, libssl3
Maintainer: $MAINTAINER
Description: LumenVision vision coprocessor backend
 Self-contained FRC vision coprocessor server (camera capture, AprilTag/object detection,
 WebRTC live preview, NetworkTables publishing). Every from-source runtime dependency
 (OpenCV, ffmpeg-rockchip, librga, etc.) is bundled privately under /opt/lumenvision - the
 only external requirements are the board's own Vulkan/Mali GPU driver and the RGA/MPP kernel
 device nodes, neither of which a .deb can provide.
EOF

cp scripts/deb/postinst "$STAGE_DIR/DEBIAN/postinst"
cp scripts/deb/prerm "$STAGE_DIR/DEBIAN/prerm"
chmod 755 "$STAGE_DIR/DEBIAN/postinst" "$STAGE_DIR/DEBIAN/prerm"

echo "==> Building $DEB_FILE"
dpkg-deb --build --root-owner-group "$STAGE_DIR" "$DEB_FILE"
echo "==> Done: $DEB_FILE"
