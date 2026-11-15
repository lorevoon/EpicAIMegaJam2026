param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot 'EpicAIMegaJam2026.uproject'
$DotNet = Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
$BuildTool = Join-Path $EngineRoot 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
$PreviousCacheRoot = $env:UBA_ROOT
try {
    # Keep the build cache in the writable project rather than ProgramData.
    $env:UBA_ROOT = Join-Path $ProjectRoot 'Saved\BuildCache'
    & $DotNet $BuildTool EpicAIMegaJam2026Editor Win64 Development "-Project=$ProjectFile" `
        -NoHotReloadFromIDE -NoUBA "-Log=$ProjectRoot\Saved\editor_build.log"
    $BuildExitCode = $LASTEXITCODE
} finally {
    $env:UBA_ROOT = $PreviousCacheRoot
}
exit $BuildExitCode
