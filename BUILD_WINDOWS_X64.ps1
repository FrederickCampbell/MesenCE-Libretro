[CmdletBinding()]
param(
    [string]$MsysRoot = "",
    [int]$Jobs = [Math]::Min(8, [Environment]::ProcessorCount),
    [switch]$Clean,
    [switch]$SkipPrerequisiteInstall
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Write-Step {
    param([string]$Message)
    Write-Host "`n=== $Message ===" -ForegroundColor Cyan
}

function Require-Command {
    param([string]$Name)

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found."
    }
}

$Root = $PSScriptRoot
$Transcript = Join-Path $Root "build-transcript.txt"
$Dist = Join-Path $Root "dist"

try {
    try { Stop-Transcript | Out-Null } catch {}
    Start-Transcript -Path $Transcript -Force | Out-Null

    if (-not $MsysRoot) {
        $Candidates = @(
            "C:\msys64"
        )

        $MsysRoot = $Candidates |
            Where-Object { Test-Path (Join-Path $_ "usr\bin\bash.exe") } |
            Select-Object -First 1
    }

    if (-not $MsysRoot) {
        if ($SkipPrerequisiteInstall) {
            throw "MSYS2 was not found."
        }

        Write-Step "Installing MSYS2"
        Require-Command "winget"
        winget install --exact --id MSYS2.MSYS2 `
            --accept-package-agreements `
            --accept-source-agreements

        $MsysRoot = "C:\msys64"
    }

    $Bash = Join-Path $MsysRoot "usr\bin\bash.exe"
    if (-not (Test-Path $Bash)) {
        throw "MSYS2 bash was not found at $Bash"
    }

    if (-not (Test-Path (Join-Path $Root "Libretro\Makefile"))) {
        throw "Libretro\Makefile was not found. Run this script from the repository root."
    }

    Write-Step "Preparing output"
    if ($Clean) {
        Get-ChildItem -LiteralPath $Root -Recurse -File -Filter "*.o" -ErrorAction SilentlyContinue |
            Remove-Item -Force

        Get-ChildItem -LiteralPath (Join-Path $Root "Libretro") -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object {
                $_.Extension -in @(".dll", ".map", ".d")
            } |
            Remove-Item -Force

        if (Test-Path $Dist) {
            Remove-Item -LiteralPath $Dist -Recurse -Force
        }
    }

    New-Item -ItemType Directory -Force -Path $Dist | Out-Null

    Write-Step "Installing MSYS2 dependencies"
    $PackageCommand = @"
set -e
pacman -Sy --noconfirm
pacman -S --needed --noconfirm \
  make \
  git \
  patch \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-binutils \
  mingw-w64-x86_64-winpthreads \
  mingw-w64-x86_64-zlib \
  mingw-w64-x86_64-libpng
"@

    & $Bash -lc $PackageCommand
    if ($LASTEXITCODE -ne 0) {
        throw "MSYS2 dependency installation failed."
    }

    $RootEscaped = $Root.Replace("'", "'\''")
    $RootUnix = (& $Bash -lc "cygpath -u '$RootEscaped'").Trim()
    if (-not $RootUnix) {
        throw "Could not translate the repository path for MSYS2."
    }

    Write-Step "Building MesenCE Libretro"
    $BuildCommand = @"
set -e
export PATH=/mingw64/bin:/usr/bin:`$PATH
cd '$RootUnix'
test -f /mingw64/lib/libwinpthread.a || {
  echo 'ERROR: libwinpthread.a is missing.' >&2
  exit 1
}
find . -type f -name '*.o' -delete
rm -f Libretro/mesen_ce_nes_libretro.dll
rm -f Libretro/mesen_libretro.dll
make --no-print-directory -C Libretro platform=win64 LTO=false -j$Jobs
"@

    & $Bash -lc $BuildCommand
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed. See build-transcript.txt."
    }

    Write-Step "Staging artifacts"
    $Dll = Get-ChildItem -LiteralPath (Join-Path $Root "Libretro") -Recurse -File `
        -Include "*_libretro.dll","mesen*.dll" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $Dll) {
        throw "The compiler returned success, but no Libretro DLL was found."
    }

    $FinalDll = Join-Path $Dist "mesen_ce_nes_libretro.dll"
    $FinalInfo = Join-Path $Dist "mesen_ce_nes_libretro.info"

    Copy-Item -LiteralPath $Dll.FullName -Destination $FinalDll -Force
    Copy-Item -LiteralPath (Join-Path $Root "info\mesen_ce_nes_libretro.info") `
        -Destination $FinalInfo `
        -Force

    $Objdump = Join-Path $MsysRoot "mingw64\bin\objdump.exe"
    if (-not (Test-Path $Objdump)) {
        throw "objdump.exe was not found at $Objdump"
    }

    $Imports = @(
        & $Objdump -p $FinalDll |
            Select-String "DLL Name:" |
            ForEach-Object {
                if ($_.Line -match "DLL Name:\s*(.+)$") {
                    $Matches[1].Trim()
                }
            } |
            Where-Object { $_ }
    )

    [System.IO.File]::WriteAllLines(
        (Join-Path $Dist "DLL_IMPORTS.txt"),
        $Imports,
        [System.Text.Encoding]::ASCII
    )

    $Forbidden = @(
        "libwinpthread-1.dll",
        "libgcc_s_seh-1.dll",
        "libstdc++-6.dll",
        "api-ms-win-crt-private-l1-1-0.dll"
    )

    $BadImports = @(
        $Imports |
            Where-Object {
                $Forbidden -contains $_.ToLowerInvariant()
            }
    )

    if ($BadImports.Count -gt 0) {
        throw "Forbidden runtime imports remain: $($BadImports -join ', ')"
    }

    Write-Step "Checking Libretro exports"
    $Handle = [IntPtr]::Zero

    try {
        $Handle = [System.Runtime.InteropServices.NativeLibrary]::Load($FinalDll)

        $RequiredExports = @(
            "retro_api_version",
            "retro_init",
            "retro_deinit",
            "retro_get_system_info",
            "retro_get_system_av_info",
            "retro_set_environment",
            "retro_set_video_refresh",
            "retro_set_audio_sample",
            "retro_set_audio_sample_batch",
            "retro_set_input_poll",
            "retro_set_input_state",
            "retro_set_controller_port_device",
            "retro_reset",
            "retro_run",
            "retro_serialize_size",
            "retro_serialize",
            "retro_unserialize",
            "retro_cheat_reset",
            "retro_cheat_set",
            "retro_load_game",
            "retro_load_game_special",
            "retro_unload_game",
            "retro_get_region",
            "retro_get_memory_data",
            "retro_get_memory_size"
        )

        $Missing = @()

        foreach ($Export in $RequiredExports) {
            $Address = [IntPtr]::Zero
            if (-not [System.Runtime.InteropServices.NativeLibrary]::TryGetExport(
                $Handle,
                $Export,
                [ref]$Address
            )) {
                $Missing += $Export
            }
        }

        if ($Missing.Count -gt 0) {
            throw "Mandatory Libretro exports are missing: $($Missing -join ', ')"
        }
    }
    finally {
        if ($Handle -ne [IntPtr]::Zero) {
            [System.Runtime.InteropServices.NativeLibrary]::Free($Handle)
        }
    }

    $Hash = (Get-FileHash -LiteralPath $FinalDll -Algorithm SHA256).Hash.ToLowerInvariant()
    [System.IO.File]::WriteAllText(
        (Join-Path $Dist "SHA256SUMS.txt"),
        "$Hash  mesen_ce_nes_libretro.dll`r`n",
        [System.Text.Encoding]::ASCII
    )

    Write-Step "Build complete"
    Write-Host "DLL:      $FinalDll" -ForegroundColor Green
    Write-Host "Core info: $FinalInfo" -ForegroundColor Green
    Write-Host "SHA-256:  $Hash" -ForegroundColor Green
}
finally {
    try { Stop-Transcript | Out-Null } catch {}
}
