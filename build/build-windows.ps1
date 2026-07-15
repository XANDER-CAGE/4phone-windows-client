[CmdletBinding()]
param(
    [ValidateSet("Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ClientsRoot = Split-Path -Parent $PSScriptRoot
$MicroSipRoot = Join-Path $ClientsRoot "microsip"
$DepsRoot = Join-Path $ClientsRoot "_deps"
$PjProjectRoot = Join-Path $DepsRoot "pjproject"
$VcpkgRoot = Join-Path $DepsRoot "vcpkg"
$VcpkgInstalledRoot = Join-Path $DepsRoot "vcpkg_installed"
$Triplet = "x86-windows-static"

$PjProjectTag = "2.17"
$PjProjectCommit = "5a457451fa2712ba18e12b01738e8ff3af2b26fd"
$PjProjectSecurityPatch = Join-Path `
    $PSScriptRoot `
    "patches\pjproject-2.17-header-boundary.patch"
$PjProjectPatchedBlob = "cdc99ff9ed5ba4d6ef2ba8c5740ff23f58c3c86c"
$VcpkgCommit = "f87344cac03158cbf1467264565f1fd36b382a24"

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Команда завершилась с кодом ${LASTEXITCODE}: $FilePath $Arguments"
    }
}

function Get-GitHead {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository
    )

    $head = & git -C $Repository rev-parse HEAD
    if ($LASTEXITCODE -ne 0) {
        throw "Не удалось прочитать HEAD репозитория $Repository"
    }
    return $head.Trim()
}

New-Item -ItemType Directory -Force -Path $DepsRoot | Out-Null

if (-not (Test-Path (Join-Path $PjProjectRoot ".git"))) {
    Invoke-NativeCommand "git" @(
        "clone",
        "--branch", $PjProjectTag,
        "--depth", "1",
        "https://github.com/pjsip/pjproject.git",
        $PjProjectRoot
    )
}

if ((Get-GitHead $PjProjectRoot) -ne $PjProjectCommit) {
    throw "В $PjProjectRoot находится другая ревизия pjproject"
}

$changedPjProjectFiles = @(
    & git -C $PjProjectRoot diff HEAD --name-only |
        Where-Object { $_ }
)
if ($LASTEXITCODE -ne 0) {
    throw "Не удалось проверить изменения pjproject"
}
if ($changedPjProjectFiles.Count -eq 0) {
    Invoke-NativeCommand "git" @(
        "-C", $PjProjectRoot,
        "apply",
        "--check",
        $PjProjectSecurityPatch
    )
    Invoke-NativeCommand "git" @(
        "-C", $PjProjectRoot,
        "apply",
        $PjProjectSecurityPatch
    )
}
elseif (
    $changedPjProjectFiles.Count -ne 1 -or
    $changedPjProjectFiles[0] -ne "pjsip/src/pjsip/sip_msg.c"
) {
    throw "В pjproject обнаружены посторонние изменения"
}
Invoke-NativeCommand "git" @(
    "-C", $PjProjectRoot,
    "apply",
    "--reverse",
    "--check",
    $PjProjectSecurityPatch
)
$patchedBlob = & git `
    -C $PjProjectRoot `
    hash-object `
    --path=pjsip/src/pjsip/sip_msg.c `
    --filters `
    "pjsip/src/pjsip/sip_msg.c"
if (
    $LASTEXITCODE -ne 0 -or
    $patchedBlob.Trim() -ne $PjProjectPatchedBlob
) {
    throw "Защитный патч pjproject не соответствует ожидаемому blob hash"
}

Copy-Item `
    -Force `
    (Join-Path $PSScriptRoot "pjproject-config_site.h") `
    (Join-Path $PjProjectRoot "pjlib\include\pj\config_site.h")

if (-not (Test-Path (Join-Path $VcpkgRoot ".git"))) {
    Invoke-NativeCommand "git" @(
        "clone",
        "https://github.com/microsoft/vcpkg.git",
        $VcpkgRoot
    )
    Invoke-NativeCommand "git" @(
        "-C", $VcpkgRoot,
        "checkout",
        "--detach",
        $VcpkgCommit
    )
}

if ((Get-GitHead $VcpkgRoot) -ne $VcpkgCommit) {
    throw "В $VcpkgRoot находится другая ревизия vcpkg"
}
$vcpkgTrackedChanges = & git `
    -C $VcpkgRoot `
    status `
    --porcelain `
    --untracked-files=no
if ($LASTEXITCODE -ne 0 -or $vcpkgTrackedChanges) {
    throw "В рабочей копии vcpkg обнаружены изменения"
}

$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    Invoke-NativeCommand `
        (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") `
        @("-disableMetrics")
}

Invoke-NativeCommand $vcpkgExe @(
    "install",
    "--triplet", $Triplet,
    "--x-manifest-root", $ClientsRoot,
    "--x-install-root", $VcpkgInstalledRoot
)

$programFilesX86 = [Environment]::GetFolderPath("ProgramFilesX86")
$vswhere = Join-Path `
    $programFilesX86 `
    "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "Visual Studio Installer и vswhere не найдены"
}

$msbuildCandidates = & $vswhere `
    -latest `
    -products * `
    -requires Microsoft.Component.MSBuild `
        Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        Microsoft.VisualStudio.Component.VC.ATLMFC `
    -find "MSBuild\**\Bin\MSBuild.exe"
if ($LASTEXITCODE -ne 0 -or -not $msbuildCandidates) {
    throw "Не найдена Visual Studio с MSBuild, C++ и MFC"
}
$msbuild = @($msbuildCandidates)[0]

$installedTripletRoot = Join-Path $VcpkgInstalledRoot $Triplet
$oldInclude = $env:INCLUDE
$oldLib = $env:LIB
$oldCl = $env:CL
try {
    $env:INCLUDE = "$(Join-Path $installedTripletRoot "include");$oldInclude"
    $env:LIB = "$(Join-Path $installedTripletRoot "lib");$oldLib"
    $opusInclude = Join-Path $installedTripletRoot "include"
    $env:CL = "/I`"$opusInclude`" $oldCl".Trim()

    Invoke-NativeCommand $msbuild @(
        (Join-Path $PjProjectRoot "pjsip-apps\build\libpjproject.vcxproj"),
        "/m",
        "/t:Build",
        "/p:Configuration=Release-Static",
        "/p:Platform=Win32"
    )

    Invoke-NativeCommand $msbuild @(
        (Join-Path $MicroSipRoot "microsip.vcxproj"),
        "/m",
        "/t:Build",
        "/p:Configuration=$Configuration",
        "/p:Platform=Win32",
        "/p:PjProjectRoot=$PjProjectRoot",
        "/p:FourPhoneDepsRoot=$installedTripletRoot"
    )
}
finally {
    $env:INCLUDE = $oldInclude
    $env:LIB = $oldLib
    $env:CL = $oldCl
}

$outputDirectory = Join-Path $MicroSipRoot $Configuration
$requiredOutputs = @(
    "4phone.exe",
    "WinSparkle.dll",
    "langpack_russian.txt",
    "langpack_uzbek.txt"
)
foreach ($requiredOutput in $requiredOutputs) {
    $outputPath = Join-Path $outputDirectory $requiredOutput
    if (-not (Test-Path $outputPath -PathType Leaf)) {
        throw "Сборка не создала обязательный файл: $outputPath"
    }
}

$executablePath = Join-Path $outputDirectory "4phone.exe"
& (Join-Path $PSScriptRoot "validate-pe-manifest.ps1") `
    -ExecutablePath $executablePath
Write-Host "Сборка 4phone завершена: $executablePath"
