# Aeternum IE-M2 Blocco1 - Protocol Adapter structural lint (IE-M3 Blocco2: + native packet hook table / dispatch).
# Rule 1 (all files under src/aeternum/protocol_adapter/**): no associative containers
#         (std::map / multimap / unordered_map / unordered_multimap / set / multiset / unordered_set / unordered_multiset,
#          nor their headers). Opcode translation must use the dense preallocated table.
# Rule 2 (hot-path header opcode_table.hpp): no heap - no new / delete-expressions /malloc/calloc/realloc/free,
#          no std::vector / std::string / std::function / make_unique / make_shared / shared_ptr / unique_ptr.
# Comments are stripped before matching (same approach as check_abi_no_stl.ps1).
# Exit 1 on any hit -> CMake target aeternum-adapter-nomap-lint fails the build (tests depend on it).
param(
  [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
  [string]$ScanRoot = ''
)

$ErrorActionPreference = 'Stop'
if (-not $ScanRoot) { $ScanRoot = Join-Path $RepoRoot 'src\aeternum\protocol_adapter' }

$mapRules = @(
  [regex]'#\s*include\s*<\s*(map|unordered_map|set|unordered_set)\s*>',
  [regex]'\bstd\s*::\s*(unordered_multimap|unordered_map|multimap|map|unordered_multiset|unordered_set|multiset|set)\b'
)
$heapRules = @(
  [regex]'\bnew\b',
  [regex]'\bdelete\s*(\[\s*\])?\s*[A-Za-z_(:]',   # delete-expression (not '= delete;')
  [regex]'\b(malloc|calloc|realloc|free)\s*\(',
  [regex]'\bstd\s*::\s*(vector|string|function|shared_ptr|unique_ptr|make_unique|make_shared)\b',
  [regex]'#\s*include\s*<\s*(vector|string|functional|memory)\s*>'
)
$hotPathNames = @('opcode_table.hpp', 'shadow.hpp', 'outbound.hpp')  # Blocco3 inbound + Blocco4 outbound run per packet
# Blocco4: worldserver glue (AdapterShadow.h/.cpp: TranslateInbound / TranslateOutbound) is also scanned for associative containers.
$glueFiles = @('src\server\game\Server\Protocol\AdapterShadow.h', 'src\server\game\Server\Protocol\AdapterShadow.cpp', 'src\server\game\Server\WorldSocket.cpp')
# IE-M3 Blocco2: native module packet hook table + guarded dispatch run per packet => no-map AND no-heap (hot path).
# IE-M3 Blocco3: WorldSocket.cpp is glue (map-only via glueFiles) — socket lifecycle uses heap; hook dispatch is header-only in packet_hooks.hpp.
$moduleHookHotFiles = @('src\modules\sdk\include\aeternum\sdk\internal\packet_hooks.hpp', 'src\modules\sdk\native\NativeHooks.cpp')
$hotPathNames += @('packet_hooks.hpp', 'NativeHooks.cpp')

function Get-CodeLines([string]$file) {
  $inBlock = $false
  $lineNo = 0
  foreach ($raw in [IO.File]::ReadAllLines($file)) {
    $lineNo++
    $line = $raw
    if ($inBlock) {
      $end = $line.IndexOf('*/')
      if ($end -ge 0) { $line = $line.Substring($end + 2); $inBlock = $false } else { continue }
    }
    while ($true) {
      $start = $line.IndexOf('/*')
      if ($start -lt 0) { break }
      $end = $line.IndexOf('*/', $start + 2)
      if ($end -ge 0) { $line = $line.Substring(0, $start) + $line.Substring($end + 2) }
      else { $line = $line.Substring(0, $start); $inBlock = $true; break }
    }
    $idx = $line.IndexOf('//')
    if ($idx -ge 0) { $line = $line.Substring(0, $idx) }
    # ignore string literals (reason codes may contain words like "free")
    $line = [regex]::Replace($line, '"([^"\\]|\\.)*"', '""')
    [pscustomobject]@{ No = $lineNo; Text = $line }
  }
}

if (-not (Test-Path $ScanRoot)) {
  Write-Host "FAIL aeternum-adapter-nomap-lint: scan root missing: $ScanRoot" -ForegroundColor Red
  exit 1
}
$files = @(Get-ChildItem -Path $ScanRoot -Recurse -Include *.h,*.hpp,*.hh,*.hxx,*.cpp,*.cc,*.inl -File)
foreach ($g in $glueFiles) {
  $gp = Join-Path $RepoRoot $g
  if (-not (Test-Path $gp)) { Write-Host "FAIL aeternum-adapter-nomap-lint: glue file missing: $g" -ForegroundColor Red; exit 1 }
  $files += Get-Item $gp
}
foreach ($g in $moduleHookHotFiles) {
  $gp = Join-Path $RepoRoot $g
  if (-not (Test-Path $gp)) { Write-Host "FAIL aeternum-adapter-nomap-lint: module hook file missing: $g" -ForegroundColor Red; exit 1 }
  $files += Get-Item $gp
}
$hotSeen = @($files | Where-Object { $hotPathNames -contains $_.Name } | ForEach-Object { $_.Name } | Sort-Object -Unique)
$hits = New-Object System.Collections.Generic.List[object]
foreach ($f in $files) {
  $isHot = $hotPathNames -contains $f.Name
  foreach ($l in (Get-CodeLines $f.FullName)) {
    foreach ($r in $mapRules) {
      if ($r.IsMatch($l.Text)) { [void]$hits.Add([pscustomobject]@{ File = $f.FullName; No = $l.No; Rule = 'no-associative-container'; Text = $l.Text.Trim() }) }
    }
    if ($isHot) {
      foreach ($r in $heapRules) {
        if ($r.IsMatch($l.Text)) { [void]$hits.Add([pscustomobject]@{ File = $f.FullName; No = $l.No; Rule = 'no-heap-hot-path'; Text = $l.Text.Trim() }) }
      }
    }
  }
}

if ($hits.Count -gt 0) {
  Write-Host "FAIL aeternum-adapter-nomap-lint:" -ForegroundColor Red
  foreach ($h in $hits) {
    $rel = $h.File
    if ($rel.StartsWith($RepoRoot)) { $rel = $rel.Substring($RepoRoot.Length).TrimStart('\','/') }
    Write-Host ("  {0}:{1}: [{2}] {3}" -f ($rel -replace '\\','/'), $h.No, $h.Rule, $h.Text)
  }
  Write-Host "Protocol adapter must use the dense preallocated AdapterOpcodeTable. See docs/m2/M2_BLOCK1.md."
  exit 1
}
Write-Host ("PASS aeternum-adapter-nomap-lint: {0} adapter file(s) scanned, no associative containers, hot path heap-free. hot-path: {1}" -f $files.Count, ($hotSeen -join ','))
exit 0
