[CmdletBinding()]
param(
    [ValidateSet("Release")]
    [string]$Configuration = "Release",

    [string]$IsccPath = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ClientsRoot = Split-Path -Parent $PSScriptRoot
$MicroSipRoot = Join-Path $ClientsRoot "microsip"
$OutputDirectory = Join-Path $MicroSipRoot $Configuration
$InstallerScript = Join-Path $ClientsRoot "installer\4phone.iss"
$ConstHeader = Join-Path $MicroSipRoot "const.h"
$InstallerPath = Join-Path $OutputDirectory "4phone-setup.exe"
$ChecksumPath = "$InstallerPath.sha256"

function Resolve-InnoCompiler {
    param(
        [string]$RequestedPath
    )

    $candidates = @()
    if ($RequestedPath) {
        $candidates += $RequestedPath
    }

    $command = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($command) {
        $candidates += $command.Source
    }

    $programFilesX86 = [Environment]::GetFolderPath("ProgramFilesX86")
    $programFiles = [Environment]::GetFolderPath("ProgramFiles")
    if ($programFilesX86) {
        $candidates += Join-Path $programFilesX86 "Inno Setup 7\ISCC.exe"
        $candidates += Join-Path $programFilesX86 "Inno Setup 6\ISCC.exe"
    }
    if ($programFiles) {
        $candidates += Join-Path $programFiles "Inno Setup 7\ISCC.exe"
        $candidates += Join-Path $programFiles "Inno Setup 6\ISCC.exe"
    }

    foreach ($candidate in ($candidates | Select-Object -Unique)) {
        if ($candidate -and (Test-Path $candidate -PathType Leaf)) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "Не найден ISCC.exe. Установите Inno Setup 6 или новее либо передайте параметр -IsccPath."
}

foreach ($requiredFile in @(
    (Join-Path $OutputDirectory "4phone.exe"),
    (Join-Path $OutputDirectory "langpack_russian.txt"),
    (Join-Path $OutputDirectory "langpack_uzbek.txt"),
    $InstallerScript,
    $ConstHeader
)) {
    if (-not (Test-Path $requiredFile -PathType Leaf)) {
        throw "Не найден обязательный файл установщика: $requiredFile"
    }
}

$constContent = Get-Content -Raw -Path $ConstHeader
$versionMatch = [regex]::Match(
    $constContent,
    '#define\s+_GLOBAL_VERSION\s+"([^"]+)"'
)
if (-not $versionMatch.Success) {
    throw "Не удалось прочитать _GLOBAL_VERSION из $ConstHeader"
}
$appVersion = $versionMatch.Groups[1].Value

$numericVersionMatch = [regex]::Match(
    $constContent,
    '#define\s+_GLOBAL_VERSION_COMMA\s+(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)'
)
if (-not $numericVersionMatch.Success) {
    throw "Не удалось прочитать _GLOBAL_VERSION_COMMA из $ConstHeader"
}
$numericVersion = (
    1..4 |
        ForEach-Object { $numericVersionMatch.Groups[$_].Value }
) -join "."

$iscc = Resolve-InnoCompiler -RequestedPath $IsccPath
Remove-Item -Force -ErrorAction SilentlyContinue $InstallerPath, $ChecksumPath

$isccArguments = @(
    "/DAppVersion=$appVersion",
    "/DAppVersionNumeric=$numericVersion",
    "/DSourceDir=$OutputDirectory",
    "/DOutputDir=$OutputDirectory",
    $InstallerScript
)
& $iscc @isccArguments
if ($LASTEXITCODE -ne 0) {
    throw "ISCC.exe завершился с кодом $LASTEXITCODE"
}

if (-not (Test-Path $InstallerPath -PathType Leaf)) {
    throw "Inno Setup не создал установщик: $InstallerPath"
}

$hash = (Get-FileHash -Algorithm SHA256 $InstallerPath).Hash.ToLowerInvariant()
$checksumLine = "$hash  $(Split-Path -Leaf $InstallerPath)`n"
[System.IO.File]::WriteAllText(
    $ChecksumPath,
    $checksumLine,
    [System.Text.Encoding]::ASCII)

Write-Host "Установщик 4phone создан: $InstallerPath"
Write-Host "SHA-256: $hash"
