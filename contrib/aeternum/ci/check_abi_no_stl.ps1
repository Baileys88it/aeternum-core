# Aeternum M1-Blocco5 — Strict ABI lint: no STL headers in C ABI interface files.
# Fails the build when ABI surface headers include STL (<string>, <vector>, ...).
# Scope (documented in M1_KERNEL_BLOCK5.md):
#   1) include/aeternum/abi/**           — C ABI (AeternumABI.h)
#   2) src/modules/**/abi/**             — future per-module ABI headers
#   3) src/modules/**/*ABI.h(pp)         — ABI-named headers under modules
# NOT scanned (STL allowed): src/modules/sdk/include (host SDK), *.cpp, admin_monitoring impl.
param(
  [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
)

$ErrorActionPreference = 'Stop'

$stlHeaders = @(
  'string', 'vector', 'map', 'unordered_map', 'set', 'list', 'deque',
  'memory', 'functional', 'optional', 'variant', 'string_view', 'array',
  'iostream', 'sstream', 'fstream', 'algorithm', 'utility', 'tuple',
  'unordered_set', 'queue', 'stack', 'bitset', 'complex', 'regex',
  'chrono', 'thread', 'mutex', 'atomic', 'condition_variable', 'future',
  'filesystem', 'span', 'any', 'type_traits', 'iterator', 'limits',
  'stdexcept', 'exception', 'new', 'typeinfo', 'initializer_list'
)

# Match #include <header> or #include <header.h> — not comments (strip // and /* */ roughly per line)
$stlAlt = ($stlHeaders | ForEach-Object { [regex]::Escape($_) }) -join '|'
$includePattern = [regex]"^\s*#\s*include\s*<\s*($stlAlt)(\.h)?\s*>"

function Get-AbiHeaders {
  param([string]$Root)
  $files = New-Object System.Collections.Generic.List[string]
  $abiRoot = Join-Path $Root 'include\aeternum\abi'
  if (Test-Path $abiRoot) {
    Get-ChildItem -Path $abiRoot -Recurse -Include *.h,*.hpp,*.hh,*.hxx -File -ErrorAction SilentlyContinue |
      ForEach-Object { [void]$files.Add($_.FullName) }
  }
  $modRoot = Join-Path $Root 'src\modules'
  if (Test-Path $modRoot) {
    Get-ChildItem -Path $modRoot -Recurse -Directory -Filter 'abi' -ErrorAction SilentlyContinue | ForEach-Object {
      Get-ChildItem -Path $_.FullName -Recurse -Include *.h,*.hpp,*.hh,*.hxx -File -ErrorAction SilentlyContinue |
        ForEach-Object { [void]$files.Add($_.FullName) }
    }
    Get-ChildItem -Path $modRoot -Recurse -Include *ABI.h,*ABI.hpp,*Abi.h,*Abi.hpp -File -ErrorAction SilentlyContinue |
      ForEach-Object { [void]$files.Add($_.FullName) }
  }
  return ($files | Select-Object -Unique)
}

function Strip-LineComment([string]$line) {
  # Remove // comments; leave string-ish content alone (ABI headers should not need strings for includes)
  $idx = $line.IndexOf('//')
  if ($idx -ge 0) { return $line.Substring(0, $idx) }
  return $line
}

$hits = New-Object System.Collections.Generic.List[object]
$headers = Get-AbiHeaders -Root $RepoRoot
if ($headers.Count -eq 0) {
  Write-Host "WARN: no ABI headers found under $RepoRoot"
}

foreach ($file in $headers) {
  $inBlock = $false
  $lineNo = 0
  foreach ($raw in [IO.File]::ReadAllLines($file)) {
    $lineNo++
    $line = $raw
    if ($inBlock) {
      $end = $line.IndexOf('*/')
      if ($end -ge 0) {
        $line = $line.Substring($end + 2)
        $inBlock = $false
      } else {
        continue
      }
    }
    while ($true) {
      $start = $line.IndexOf('/*')
      if ($start -lt 0) { break }
      $end = $line.IndexOf('*/', $start + 2)
      if ($end -ge 0) {
        $line = $line.Substring(0, $start) + $line.Substring($end + 2)
      } else {
        $line = $line.Substring(0, $start)
        $inBlock = $true
        break
      }
    }
    $line = Strip-LineComment $line
    if ($includePattern.IsMatch($line)) {
      $rel = $file
      if ($rel.StartsWith($RepoRoot)) { $rel = $rel.Substring($RepoRoot.Length).TrimStart('\','/') }
      $rel = $rel -replace '\\','/'
      [void]$hits.Add([pscustomobject]@{ Path = $rel; Line = $lineNo; Text = $line.Trim() })
    }
  }
}

if ($hits.Count -gt 0) {
  Write-Host "FAIL aeternum-abi-stl-lint: STL include(s) in ABI interface header(s):" -ForegroundColor Red
  foreach ($h in $hits) {
    Write-Host ("  {0}:{1}: {2}" -f $h.Path, $h.Line, $h.Text)
  }
  Write-Host "ABI headers must use C11 headers only (<stdint.h>, <stddef.h>, ...). See M1_KERNEL_BLOCK5.md."
  exit 1
}

Write-Host ("PASS aeternum-abi-stl-lint: {0} ABI header(s) scanned, no STL includes." -f $headers.Count)
exit 0
