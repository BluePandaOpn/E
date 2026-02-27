Set-StrictMode -Version Latest

function Get-TkinterTree {
    [CmdletBinding()]
    param(
        [string]$Root = (Split-Path -Parent $PSScriptRoot)
    )

    Get-ChildItem -Path $Root -Recurse -File |
        ForEach-Object { $_.FullName.Replace($Root, ".") } |
        Sort-Object
}

function Test-TkinterLayout {
    [CmdletBinding()]
    param(
        [string]$Root = (Split-Path -Parent $PSScriptRoot)
    )

    $required = @(
        "__init__.epp",
        "tk.epp",
        "core.epp",
        "helpers.epp",
        "runtime.epp",
        "tkinter.epp",
        "src/epp/core.epp",
        "src/epp/helpers.epp",
        "src/epp/runtime.epp",
        "src/epp/tk.epp",
        "src/cpp/tk_native_bridge.hpp",
        "src/cpp/tk_native_bridge.cpp",
        "_native/README.md",
        "docs/ARCHITECTURE.md",
        "docs/API.md",
        "README.md"
    )

    $missing = @()
    foreach ($item in $required) {
        $path = Join-Path $Root $item
        if (-not (Test-Path -Path $path)) {
            $missing += $item
        }
    }

    if ($missing.Count -eq 0) {
        [PSCustomObject]@{
            Ok      = $true
            Missing = @()
        }
    }
    else {
        [PSCustomObject]@{
            Ok      = $false
            Missing = $missing
        }
    }
}

Export-ModuleMember -Function Get-TkinterTree, Test-TkinterLayout
