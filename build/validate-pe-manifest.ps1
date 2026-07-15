[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ExecutablePath
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$resolvedExecutable = (Resolve-Path -LiteralPath $ExecutablePath).Path
$windowsKitsBin = Join-Path `
    ${env:ProgramFiles(x86)} `
    "Windows Kits\10\bin"
$manifestTools = @(
    Get-ChildItem `
        -Path $windowsKitsBin `
        -Filter "mt.exe" `
        -File `
        -Recurse |
        Where-Object { $_.Directory.Name -eq "x86" } |
        Sort-Object -Property FullName -Descending
)
if ($manifestTools.Count -eq 0) {
    throw "Не найден x86 mt.exe из Windows SDK"
}

$manifestTool = $manifestTools[0].FullName
$manifestPath = Join-Path `
    $env:TEMP `
    "4phone-$([Guid]::NewGuid().ToString('N')).manifest"

try {
    & $manifestTool `
        -nologo `
        "-inputresource:$resolvedExecutable;#1" `
        "-out:$manifestPath"
    if ($LASTEXITCODE -ne 0) {
        throw "Не удалось извлечь manifest из $resolvedExecutable"
    }

    [xml]$manifest = Get-Content -Raw -LiteralPath $manifestPath
    $namespaces = [System.Xml.XmlNamespaceManager]::new(
        $manifest.NameTable
    )
    $namespaces.AddNamespace(
        "asmv3",
        "urn:schemas-microsoft-com:asm.v3"
    )
    $namespaces.AddNamespace(
        "ws2005",
        "http://schemas.microsoft.com/SMI/2005/WindowsSettings"
    )
    $namespaces.AddNamespace(
        "ws2016",
        "http://schemas.microsoft.com/SMI/2016/WindowsSettings"
    )

    $dpiAware = @(
        $manifest.SelectNodes(
            "//asmv3:windowsSettings/ws2005:dpiAware",
            $namespaces
        )
    )
    if ($dpiAware.Count -ne 1) {
        throw (
            "Manifest должен содержать ровно один dpiAware, найдено: " +
            $dpiAware.Count
        )
    }
    if ($dpiAware[0].InnerText.Trim() -ne "true/pm") {
        throw "dpiAware должен иметь значение true/pm"
    }

    $dpiAwareness = @(
        $manifest.SelectNodes(
            "//asmv3:windowsSettings/ws2016:dpiAwareness",
            $namespaces
        )
    )
    if ($dpiAwareness.Count -ne 1) {
        throw (
            "Manifest должен содержать ровно один dpiAwareness, найдено: " +
            $dpiAwareness.Count
        )
    }
    if (
        $dpiAwareness[0].InnerText.Trim() -ne
            "PerMonitorV2,PerMonitor"
    ) {
        throw "dpiAwareness содержит неожиданное значение"
    }

    $dynamicRuntimeAssemblies = @(
        $manifest.SelectNodes(
            "//*[local-name()='assemblyIdentity']"
        ) |
            Where-Object {
                $_.GetAttribute("name") -match
                    "^Microsoft\.(?:VC|MFC)"
            }
    )
    if ($dynamicRuntimeAssemblies.Count -ne 0) {
        $names = $dynamicRuntimeAssemblies |
            ForEach-Object { $_.GetAttribute("name") }
        throw (
            "Статическая сборка неожиданно требует SxS runtime: " +
            ($names -join ", ")
        )
    }

    Write-Host "PE manifest 4phone проверен: $resolvedExecutable"
}
finally {
    Remove-Item `
        -LiteralPath $manifestPath `
        -Force `
        -ErrorAction SilentlyContinue
}
