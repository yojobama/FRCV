<#
.SYNOPSIS
  Builds and deploys FRCV to the Orange Pi.

.DESCRIPTION
  Run this FROM WINDOWS after Visual Studio has already built FRCVLib for ARM64 (Release, via
  the Remote_GCC toolset - F5/Build on that configuration puts libFRCVLib.so on the Pi already,
  under RemoteRootDir/FRCVLib/bin/ARM64/Release/ by default). This script:
    1. Publishes Server self-contained for linux-arm64 (so the Pi needs no .NET install at all)
    2. Builds the WebUI (reactproject1) and folds it into the publish output's wwwroot
    3. Copies both, plus the already-built libFRCVLib.so, to /opt/frcv on the Pi
    4. Installs/refreshes the systemd unit and restarts the service

.PARAMETER PiHost
  Orange Pi hostname or IP.
.PARAMETER PiUser
  SSH user on the Pi.
.PARAMETER RemoteRootDir
  Must match FRCVLib/Local.props' RemoteRootDir for the ARM64 platform - where Visual Studio's
  Remote_GCC toolset put the built libFRCVLib.so.
.PARAMETER SkipWebUI
  Skip building/deploying the WebUI (useful when only the server changed).

.EXAMPLE
  ./scripts/deploy.ps1 -PiHost 192.168.55.138 -PiUser ubuntu
#>
param(
    [string]$PiHost = "192.168.55.138",
    [string]$PiUser = "ubuntu",
    [string]$RemoteRootDir = "/home/ubuntu/frcv-remote-build",
    [switch]$SkipWebUI,
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$remoteDeployDir = "/opt/frcv"
$publishDir = Join-Path $repoRoot "Server/bin/$Configuration/net10.0/linux-arm64/publish"

function Invoke-Remote([string]$Command) {
    ssh "$PiUser@$PiHost" $Command
}

Write-Host "==> Publishing Server (self-contained, linux-arm64, $Configuration)" -ForegroundColor Cyan
dotnet publish (Join-Path $repoRoot "Server/Server.csproj") `
    -c $Configuration -r linux-arm64 --self-contained true `
    -p:FRCVLibPlatform=ARM64 -p:FRCVLibConfiguration=$Configuration
if ($LASTEXITCODE -ne 0) { throw "dotnet publish failed" }

if (-not $SkipWebUI) {
    $webUiDir = Join-Path $repoRoot "reactproject1"
    if (Test-Path $webUiDir) {
        Write-Host "==> Building WebUI" -ForegroundColor Cyan
        Push-Location $webUiDir
        try {
            npm run build
            if ($LASTEXITCODE -ne 0) { throw "npm run build failed" }
            $wwwroot = Join-Path $publishDir "wwwroot"
            New-Item -ItemType Directory -Force -Path $wwwroot | Out-Null
            Copy-Item -Recurse -Force (Join-Path $webUiDir "dist\*") $wwwroot
        } finally {
            Pop-Location
        }
    } else {
        Write-Warning "reactproject1 not found, skipping WebUI build"
    }
}

Write-Host "==> Locating the Visual Studio remote build's libFRCVLib.so on the Pi" -ForegroundColor Cyan
$remoteSoPath = "$RemoteRootDir/FRCVLib/bin/ARM64/$Configuration/libFRCVLib.so"
$soCheck = ssh "$PiUser@$PiHost" "test -f '$remoteSoPath' && echo FOUND || echo MISSING"
if ($soCheck.Trim() -ne "FOUND") {
    throw "libFRCVLib.so not found at $remoteSoPath on the Pi - build the ARM64/$Configuration configuration in Visual Studio first (Remote_GCC toolset), or pass -RemoteRootDir matching Local.props"
}

Write-Host "==> Copying published server to $PiUser@$PiHost`:$remoteDeployDir" -ForegroundColor Cyan
Invoke-Remote "sudo mkdir -p $remoteDeployDir && sudo chown ${PiUser}:${PiUser} $remoteDeployDir"
scp -r "$publishDir/*" "${PiUser}@${PiHost}:$remoteDeployDir/"
Invoke-Remote "cp '$remoteSoPath' $remoteDeployDir/libFRCVLib.so && chmod +x $remoteDeployDir/Server"

Write-Host "==> Installing systemd unit" -ForegroundColor Cyan
$serviceFile = Join-Path $repoRoot "scripts/frcv.service"
scp $serviceFile "${PiUser}@${PiHost}:/tmp/frcv.service"
Invoke-Remote "sudo id frcv >/dev/null 2>&1 || sudo useradd --system --no-create-home --shell /usr/sbin/nologin frcv; sudo usermod -aG video,render frcv; sudo chown -R frcv:frcv $remoteDeployDir; sudo mv /tmp/frcv.service /etc/systemd/system/frcv.service; sudo systemctl daemon-reload; sudo systemctl enable frcv; sudo systemctl restart frcv"

Write-Host "==> Deployed. Status:" -ForegroundColor Green
Invoke-Remote "sudo systemctl status frcv --no-pager -l | head -15"
