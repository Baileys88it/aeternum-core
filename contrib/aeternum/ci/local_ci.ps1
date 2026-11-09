<#
.SYNOPSIS
  Local CI for Aeternum Engine (no GitHub Actions).

.DESCRIPTION
  Incremental RelWithDebInfo build of worldserver, bnetserver, and tests,
  then runs Catch2 tests.exe. Does NOT touch Trinity / <SERVER_ROOT>,
  does NOT enable GitHub Actions, does NOT deploy.

.EXAMPLE
  powershell -NoProfile -File contrib/aeternum/ci/local_ci.ps1
  powershell -NoProfile -File contrib/aeternum/ci/local_ci.ps1 -SkipBuild
  powershell -NoProfile -File contrib/aeternum/ci/local_ci.ps1 -Filter "[classic]"
#>
param(
  [string]$SourceDir = '',
  [string]$BuildDir = '',
  [string]$Config = 'RelWithDebInfo',
  [string]$Filter = '*',
  [switch]$SkipBuild,
  [switch]$Reconfigure
)

$ErrorActionPreference = 'Stop'
if (-not $SourceDir) {
  # repo root = three levels up from this script: contrib/aeternum/ci
  $SourceDir = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
}
if (-not $BuildDir) {
  $BuildDir = Join-Path (Split-Path $SourceDir -Parent) 'build'
  if (-not (Test-Path $BuildDir)) { $BuildDir = Join-Path $SourceDir 'build' }
}

$cmake = $null
foreach ($c in @('C:\Program Files\CMake\bin\cmake.exe', 'cmake')) {
  if ($c -eq 'cmake') { $cmake = (Get-Command cmake -EA SilentlyContinue).Source; break }
  if (Test-Path $c) { $cmake = $c; break }
}
if (-not $cmake) { throw 'cmake not found' }

$testsExe = Join-Path $BuildDir "bin\$Config\tests.exe"
$logDir = Join-Path (Split-Path $BuildDir -Parent) '_buildlogs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$log = Join-Path $logDir "local_ci_$stamp.log"

function Log([string]$msg) {
  $line = "[{0}] {1}" -f (Get-Date -Format s), $msg
  Write-Host $line
  Add-Content -Path $log -Value $line -Encoding UTF8
}

Log "Aeternum local CI start"
Log "Source=$SourceDir"
Log "Build=$BuildDir Config=$Config Filter=$Filter"
Log "Log=$log"

if ($Reconfigure) {
  Log 'Reconfigure...'
  & $cmake -S $SourceDir -B $BuildDir 2>&1 | Tee-Object -FilePath $log -Append | Out-Null
  if ($LASTEXITCODE -ne 0) { throw "cmake configure failed: $LASTEXITCODE" }
}

if (-not $SkipBuild) {
  Log 'Build worldserver bnetserver tests...'
  & $cmake --build $BuildDir --config $Config --target worldserver,bnetserver,tests -j 8 2>&1 |
    Tee-Object -FilePath $log -Append | Select-Object -Last 40
  if ($LASTEXITCODE -ne 0) { throw "build failed: $LASTEXITCODE" }
  Log 'Build OK'
} else {
  Log 'SkipBuild set'
}

if (-not (Test-Path $testsExe)) { throw "tests.exe missing: $testsExe" }

Log "Run Catch2: $testsExe $Filter"
$outXml = Join-Path $logDir "local_ci_$stamp.junit.xml"
& $testsExe $Filter --reporter junit --out $outXml 2>&1 | Tee-Object -FilePath $log -Append
$testEc = $LASTEXITCODE
Log "tests exit=$testEc junit=$outXml"

# Smoke: runtime helpers present after build
$runtime = Join-Path $BuildDir "bin\$Config"
foreach ($f in @('START_SERVER.bat','STOP_SERVER.bat','STATUS.bat','CHECK_CONFIG.bat','README.txt','legacy.dll')) {
  $p = Join-Path $runtime $f
  if (Test-Path $p) { Log "artifact OK $f" } else { Log "artifact MISSING $f"; $testEc = 1 }
}

if ($testEc -ne 0) {
  Log "LOCAL CI FAILED ($testEc)"
  exit $testEc
}
Log 'LOCAL CI PASSED'
exit 0
