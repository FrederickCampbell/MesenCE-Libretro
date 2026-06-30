[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string]$RetroArchPath
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RetroArchPath = (Resolve-Path -LiteralPath $RetroArchPath).Path
$Dll = Join-Path $PSScriptRoot "dist\mesen_ce_nes_libretro.dll"
$Info = Join-Path $PSScriptRoot "dist\mesen_ce_nes_libretro.info"

if (-not (Test-Path $Dll)) {
    throw "Missing $Dll. Run BUILD_WINDOWS_X64.ps1 first."
}

if (-not (Test-Path $Info)) {
    throw "Missing $Info. Run BUILD_WINDOWS_X64.ps1 first."
}

$CoreDir = Join-Path $RetroArchPath "cores"
$InfoDir = Join-Path $RetroArchPath "info"

New-Item -ItemType Directory -Force -Path $CoreDir, $InfoDir | Out-Null

$TargetDll = Join-Path $CoreDir "mesen_ce_nes_libretro.dll"
$TargetInfo = Join-Path $InfoDir "mesen_ce_nes_libretro.info"

if (Test-Path $TargetDll) {
    $Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    Copy-Item -LiteralPath $TargetDll `
        -Destination "$TargetDll.backup-$Stamp" `
        -Force
}

Copy-Item -LiteralPath $Dll -Destination $TargetDll -Force
Copy-Item -LiteralPath $Info -Destination $TargetInfo -Force

Write-Host "Installed MesenCE Libretro:" -ForegroundColor Green
Write-Host $TargetDll
Write-Host "The stock mesen_libretro.dll was not modified." -ForegroundColor Yellow