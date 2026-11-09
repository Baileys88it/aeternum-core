# Deploy-AeternumModuleManifests.ps1
# Aeternum M1 (D9): copy every <SourceModulesDir>/<id>/module.manifest.json into the manifest directory that the
# worldserver of an instance with its own conf folder will read at boot:
#   Aeternum.Modules.ManifestDir from -ConfFile when set, otherwise <directory of -ConfFile>/modules
#   (same rule as TrinityOpsAdapter ResolveManifestBootOptions).
# Manifests are read once at boot (on-boot-only), so copying while the server runs is harmless; restart to apply.
# Never deletes files in the target; stale manifests are only reported. -DryRun prints the plan.
param(
  [Parameter(Mandatory=$true)][string]$ConfFile,
  [string]$SourceModulesDir = (Join-Path $PSScriptRoot 'modules'),
  [string]$RuntimeDir = '',
  [switch]$DryRun
)
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $ConfFile -PathType Leaf)) { Write-Error "ConfFile not found: $ConfFile"; exit 2 }
if (-not (Test-Path -LiteralPath $SourceModulesDir -PathType Container)) { Write-Error "SourceModulesDir not found: $SourceModulesDir"; exit 2 }
$conf = [IO.Path]::GetFullPath($ConfFile)

# Last non-comment assignment wins (TrinityCore config semantics).
$manifestDir = ''
foreach ($line in [IO.File]::ReadAllLines($conf)) {
  if ($line -match '^\s*Aeternum\.Modules\.ManifestDir\s*=\s*"?([^"]*)"?\s*$') { $manifestDir = $Matches[1].Trim() }
}
if ([string]::IsNullOrEmpty($manifestDir)) {
  $target = Join-Path (Split-Path -Parent $conf) 'modules'
} elseif ([IO.Path]::IsPathRooted($manifestDir)) {
  $target = $manifestDir
} else {
  # Relative ManifestDir is resolved by worldserver against its working directory (the runtime/bin folder).
  if ([string]::IsNullOrEmpty($RuntimeDir)) { Write-Error "Aeternum.Modules.ManifestDir '$manifestDir' is relative: pass -RuntimeDir (worldserver working dir)"; exit 2 }
  $target = Join-Path $RuntimeDir $manifestDir
}
$target = [IO.Path]::GetFullPath($target)
Write-Host "Manifest source: $([IO.Path]::GetFullPath($SourceModulesDir))"
Write-Host "Manifest target: $target"

$copied = 0
$srcIds = @()
foreach ($dir in Get-ChildItem -LiteralPath $SourceModulesDir -Directory) {
  $file = Join-Path $dir.FullName 'module.manifest.json'
  if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { continue }
  if ($dir.Name -notmatch '^[a-z0-9_-]+(\.[a-z0-9_-]+)*$') { Write-Warning "Skip unsafe module id folder '$($dir.Name)'"; continue }
  $srcIds += $dir.Name
  $dst = Join-Path (Join-Path $target $dir.Name) 'module.manifest.json'
  if ($DryRun) { Write-Host "DRYRUN copy $($dir.Name) -> $dst"; continue }
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $dst) | Out-Null
  Copy-Item -LiteralPath $file -Destination $dst -Force
  $copied++
  Write-Host "copied $($dir.Name) -> $dst"
}
if (Test-Path -LiteralPath $target -PathType Container) {
  foreach ($d in Get-ChildItem -LiteralPath $target -Directory) {
    if ($srcIds -notcontains $d.Name -and (Test-Path -LiteralPath (Join-Path $d.FullName 'module.manifest.json'))) {
      Write-Warning "Target has manifest not present in source (kept): $($d.Name)"
    }
  }
}
if ($srcIds.Count -eq 0) { Write-Warning "No module manifests found under $SourceModulesDir" }
Write-Host "Manifests deployed: $copied$(if ($DryRun) { ' (dry run)' })"
exit 0