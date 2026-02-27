Set-StrictMode -Version Latest

function Test-MathLibLayout {
    [CmdletBinding()]
    param(
        [string]$Root = (Split-Path -Parent $PSScriptRoot)
    )

    $required = @(
        "__init__.epp",
        "math.epp",
        "src/main.epp",
        "src/math.epp",
        "docs/README.es.md",
        "docs/API.md",
        "docs/CPLUSPLUS_INTEGRATION.md"
    )

    $result = foreach ($item in $required) {
        $full = Join-Path $Root $item
        [PSCustomObject]@{
            File   = $item
            Exists = Test-Path -LiteralPath $full
        }
    }

    $missing = $result | Where-Object { -not $_.Exists }
    if ($missing) {
        Write-Warning "Faltan archivos requeridos para stdlib.math."
    } else {
        Write-Output "Layout OK: estructura base valida."
    }

    return $result
}

function New-MathNativeBridgeStub {
    [CmdletBinding()]
    param(
        [string]$OutFile = "math_bridge_stub.cpp"
    )

    $content = @'
#include <cmath>

extern "C" {
    double math_pi() { return 3.14159265358979323846; }
    double math_e() { return 2.71828182845904523536; }
    double math_sqrt(double x) { return std::sqrt(x); }
    double math_pow(double a, double b) { return std::pow(a, b); }
    double math_floor(double x) { return std::floor(x); }
    double math_ceil(double x) { return std::ceil(x); }
    double math_sin(double x) { return std::sin(x); }
    double math_cos(double x) { return std::cos(x); }
    double math_tan(double x) { return std::tan(x); }
    double math_log(double x) { return std::log(x); }
    double math_exp(double x) { return std::exp(x); }
}
'@

    Set-Content -Path $OutFile -Value $content -Encoding UTF8
    Write-Output "Bridge stub generado en: $OutFile"
}

Export-ModuleMember -Function Test-MathLibLayout, New-MathNativeBridgeStub
