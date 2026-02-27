# color.psm1
# Logica principal de colores en consola (true color ANSI)

Set-StrictMode -Version Latest

function Test-HexColor {
    param(
        [Parameter(Mandatory=$true)][string]$Hex
    )

    return $Hex -match '^#(?:[0-9a-fA-F]{6})$'
}

function Convert-HexToRgb {
    param(
        [Parameter(Mandatory=$true)][string]$Hex
    )

    if (-not (Test-HexColor -Hex $Hex)) {
        throw "Formato de color invalido: '$Hex'. Usa #RRGGBB."
    }

    $r = [Convert]::ToInt32($Hex.Substring(1,2),16)
    $g = [Convert]::ToInt32($Hex.Substring(3,2),16)
    $b = [Convert]::ToInt32($Hex.Substring(5,2),16)

    return [ordered]@{ R = $r; G = $g; B = $b }
}

function Get-AnsiForeground {
    param(
        [Parameter(Mandatory=$true)][string]$Hex
    )

    $rgb = Convert-HexToRgb -Hex $Hex
    return [string]([char]27) + "[38;2;$($rgb.R);$($rgb.G);$($rgb.B)m"
}

function Get-AnsiBackground {
    param(
        [Parameter(Mandatory=$true)][string]$Hex
    )

    $rgb = Convert-HexToRgb -Hex $Hex
    return [string]([char]27) + "[48;2;$($rgb.R);$($rgb.G);$($rgb.B)m"
}

function Get-AnsiReset {
    return [string]([char]27) + "[0m"
}

function Write-ColorText {
    param(
        [Parameter(Mandatory=$true)][string]$Text,
        [Parameter(Mandatory=$true)][string]$ForegroundHex,
        [string]$BackgroundHex
    )

    $fg = Get-AnsiForeground -Hex $ForegroundHex
    $bg = ''
    if ($BackgroundHex) {
        $bg = Get-AnsiBackground -Hex $BackgroundHex
    }

    $reset = Get-AnsiReset
    Write-Host "$fg$bg$Text$reset"
}

Export-ModuleMember -Function Test-HexColor,Convert-HexToRgb,Get-AnsiForeground,Get-AnsiBackground,Get-AnsiReset,Write-ColorText
