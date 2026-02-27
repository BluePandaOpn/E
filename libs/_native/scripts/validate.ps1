$ErrorActionPreference = "Stop"

Write-Host "[validate] checking _native layout..."

$requiredPaths = @(
  "__init__.epp",
  "_native.epp",
  "src\_native.epp",
  "docs\API.md",
  "docs\ARCHITECTURE.md"
)

foreach ($p in $requiredPaths) {
  if (-not (Test-Path $p)) {
    throw "[validate] missing required file: $p"
  }
}

$requiredFunctions = @(
  "builtin_mode",
  "runtime_name",
  "module_name",
  "module_version",
  "api_version",
  "is_stable",
  "build_target",
  "health_check",
  "diagnostics_hint",
  "capabilities_text"
)

$rootContent = Get-Content "_native.epp" -Raw
$srcContent = Get-Content "src\_native.epp" -Raw

foreach ($fn in $requiredFunctions) {
  if ($rootContent -notmatch ("func\s+" + [Regex]::Escape($fn) + "\s*\(")) {
    throw "[validate] function missing in _native.epp: $fn"
  }
  if ($srcContent -notmatch ("func\s+" + [Regex]::Escape($fn) + "\s*\(")) {
    throw "[validate] function missing in src\_native.epp: $fn"
  }
}

$rootVersion = [regex]::Match($rootContent, 'func\s+module_version\(\)\s*\{\s*return\s+"([^"]+)"').Groups[1].Value
$srcVersion = [regex]::Match($srcContent, 'func\s+module_version\(\)\s*\{\s*return\s+"([^"]+)"').Groups[1].Value

if ([string]::IsNullOrWhiteSpace($rootVersion) -or [string]::IsNullOrWhiteSpace($srcVersion)) {
  throw "[validate] unable to read module_version from one or more files"
}

if ($rootVersion -ne $srcVersion) {
  throw "[validate] version mismatch: _native.epp=$rootVersion src\\_native.epp=$srcVersion"
}

Write-Host "[validate] OK - structure and API are consistent."
