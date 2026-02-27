Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Invoke-HttpLibCheck {
    [CmdletBinding()]
    param(
        [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    )

    $required = @(
        "http.epp",
        "__init__.epp",
        "src\http_core.epp",
        "src\native\http_bridge.cpp",
        "README.md"
    )

    foreach ($path in $required) {
        $full = Join-Path $Root $path
        if (-not (Test-Path -LiteralPath $full)) {
            throw "Falta archivo requerido: $path"
        }
    }

    [PSCustomObject]@{
        Name = "stdlib.http"
        Root = $Root
        Status = "OK"
    }
}

function Invoke-HttpLibPackage {
    [CmdletBinding()]
    param(
        [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
        [string]$OutDir = (Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "dist")
    )

    if (-not (Test-Path -LiteralPath $OutDir)) {
        New-Item -ItemType Directory -Path $OutDir | Out-Null
    }

    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $zipPath = Join-Path $OutDir "stdlib.http-$stamp.zip"

    $items = @(
        (Join-Path $Root "http.epp"),
        (Join-Path $Root "__init__.epp"),
        (Join-Path $Root "README.md"),
        (Join-Path $Root "src"),
        (Join-Path $Root "docs")
    ) | Where-Object { Test-Path -LiteralPath $_ }

    Compress-Archive -Path $items -DestinationPath $zipPath -Force
    return $zipPath
}

Export-ModuleMember -Function Invoke-HttpLibCheck, Invoke-HttpLibPackage
