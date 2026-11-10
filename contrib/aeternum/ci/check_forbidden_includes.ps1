# IE-M1 — forbidden Trinity types in public API / SDK include trees.
# Fails on include/aeternum/api and src/modules/sdk/include.
# Reports DEBT (exit 0) for host/admin coupling with categorized hits.
# ALLOWED coupling seat (NOT scanned): src/modules/sdk/adapters/trinity
param(
  [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
  [string]$BacklogOut = ''
)

$ErrorActionPreference = 'Stop'
$failPaths = @(
  (Join-Path $RepoRoot 'include\aeternum\api'),
  (Join-Path $RepoRoot 'include\aeternum\abi'),
  (Join-Path $RepoRoot 'src\modules\sdk\include')
)
# Host + admin only. Trinity adapter under adapters/trinity is the permitted seat.
$debtPaths = @(
  (Join-Path $RepoRoot 'src\modules\sdk\src'),
  (Join-Path $RepoRoot 'src\modules\admin_monitoring')
)

# Public-surface hard ban (types + includes)
$failPattern = 'Player\s*\*|WorldSession\s*\*|Map\s*\*|Unit\s*\*|Creature\s*\*|WorldObject\s*\*|#include\s*<[^>]*Player\.h>|#include\s*"([^"]*/)?Player\.h"|#include\s*<[^>]*WorldSession\.h>|#include\s*"([^"]*/)?WorldSession\.h"|#include\s*<[^>]*\bMap\.h>|#include\s*"([^"]*/)?Map\.h"|#include\s*<[^>]*Unit\.h>|#include\s*"([^"]*/)?Unit\.h"|#include\s*<[^>]*Creature\.h>|#include\s*"([^"]*/)?Creature\.h"|#include\s*<[^>]*WorldObject\.h>|#include\s*"([^"]*/)?WorldObject\.h"|#include\s*<[^>]*\bWorld\.h>|#include\s*"([^"]*/)?World\.h"'

# Debt markers for host/admin coupling to game/Trinity internals
$debtPattern = '#include\s*"(World|Player|Unit|Creature|Map|WorldSession|WorldObject|ObjectAccessor|ScriptMgr|DatabaseEnv|GameTime|ObjectMgr|Opcodes|Metric|MetricLocalStore)\.h"'

function Get-Hits {
  param([string[]]$Roots, [string]$Pattern)
  $hits = @()
  foreach ($r in $Roots) {
    if (-not (Test-Path $r)) { continue }
    Get-ChildItem $r -Recurse -Include *.h,*.hpp,*.cpp,*.cc,*.cxx -File -ErrorAction SilentlyContinue | ForEach-Object {
      $m = Select-String -Path $_.FullName -Pattern $Pattern -AllMatches -ErrorAction SilentlyContinue
      if ($m) { $hits += $m }
    }
  }
  return $hits
}

$failHits = Get-Hits -Roots $failPaths -Pattern $failPattern
if ($failHits.Count -gt 0) {
  Write-Host "FAIL forbidden-include hits: $($failHits.Count)"
  $failHits | Select-Object -First 40 | ForEach-Object { Write-Host $_.Path ':' $_.LineNumber ':' $_.Line.Trim() }
  exit 2
}
Write-Host "PASS public surfaces: $($failPaths -join ', ')"

$debtHits = Get-Hits -Roots $debtPaths -Pattern $debtPattern
Write-Host "DEBT coupling hits: $($debtHits.Count)"
$debtHits | ForEach-Object { Write-Host $_.Path ':' $_.LineNumber ':' $_.Line.Trim() }

$adapterRoot = Join-Path $RepoRoot 'src\modules\sdk\adapters\trinity'
$adapterHits = @()
if (Test-Path $adapterRoot) {
  $adapterHits = Get-Hits -Roots @($adapterRoot) -Pattern $debtPattern
  Write-Host "ADAPTER seat includes (allowed): $($adapterHits.Count)"
  $adapterHits | ForEach-Object { Write-Host '  ' $_.Path ':' $_.LineNumber ':' $_.Line.Trim() }
}

if ($BacklogOut) {
  $dir = Split-Path -Parent $BacklogOut
  if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
  $lines = New-Object System.Collections.Generic.List[string]
  [void]$lines.Add('# IE Coupling Backlog (generated)')
  [void]$lines.Add('')
  [void]$lines.Add(('**Generated:** {0} (machine local)' -f (Get-Date -Format 'yyyy-MM-dd HH:mm')))
  [void]$lines.Add(('**RepoRoot:** `{0}`' -f $RepoRoot))
  [void]$lines.Add('**Public surface:** PASS (no forbidden types/includes in `include/aeternum/api` / `src/modules/sdk/include`).')
  [void]$lines.Add('**Allowed seat:** `src/modules/sdk/adapters/trinity` (TrinityCore adapter for SDK 1.0 ops).')
  [void]$lines.Add('')
  [void]$lines.Add('## Known debt (host / admin)')
  [void]$lines.Add('')
  [void]$lines.Add('These includes couple Module SDK host and Admin monitoring to Trinity/game internals. Expected until Operations / Module extraction. Do not expand.')
  [void]$lines.Add('')
  if ($debtHits.Count -eq 0) {
    [void]$lines.Add('_No debt markers matched — host/admin are clean. Trinity includes belong only under adapters/trinity._')
  } else {
    $byFile = $debtHits | Group-Object Path
    foreach ($g in ($byFile | Sort-Object Name)) {
      $rel = $g.Name
      if ($rel.StartsWith($RepoRoot)) { $rel = $rel.Substring($RepoRoot.Length).TrimStart('\','/') }
      $rel = $rel -replace '\\','/'
      [void]$lines.Add(('### `{0}`' -f $rel))
      [void]$lines.Add('')
      foreach ($h in $g.Group) {
        [void]$lines.Add(('- L{0}: `{1}`' -f $h.LineNumber, $h.Line.Trim()))
      }
      [void]$lines.Add('')
    }
  }
  [void]$lines.Add('## Clearance criteria')
  [void]$lines.Add('')
  [void]$lines.Add('- Host uses `aeternum/api` + ops ports only (no `World.h` / `ScriptMgr.h`).')
  [void]$lines.Add('- Admin HTTP talks to `IRuntimeStats` / `IMetricLocalQuery` / `IProcessControl` / `IDatabasePing` via `aeternum::ops::Services()`.')
  [void]$lines.Add('- Trinity coupling confined to `src/modules/sdk/adapters/trinity`.')
  [void]$lines.Add('- Debt count must trend down; new hits in host/admin trees are regressions.')
  $utf8enc = New-Object System.Text.UTF8Encoding $false
  [IO.File]::WriteAllLines($BacklogOut, $lines, $utf8enc)
  Write-Host "Wrote backlog: $BacklogOut"
}

Write-Host 'check_forbidden_includes: done'
exit 0
