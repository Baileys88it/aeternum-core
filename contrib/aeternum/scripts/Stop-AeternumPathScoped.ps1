# Stop-AeternumPathScoped.ps1
# Graceful stop of bnetserver/worldserver whose ExecutablePath is under -RuntimeDir.
# NEVER kills by process name alone. Announce IT+EN + "server shutdown <delay>" to worldserver
# console, wait, then Stop-Process -Force only on those PIDs. DryRun: plan only.
param(
  [Parameter(Mandatory=$true)][string]$RuntimeDir,
  [int]$ShutdownDelay = 120,
  [int]$ExitTimeoutSec = 180,
  [string[]]$WindowTitlePrefixes = @('Aeternum-bnetserver','Aeternum-worldserver'),
  [switch]$DryRun
)
$ErrorActionPreference = 'Continue'
$here = [IO.Path]::GetFullPath($RuntimeDir).TrimEnd('\')

function Get-Scoped {
  Get-CimInstance Win32_Process -Filter "Name='worldserver.exe' OR Name='bnetserver.exe'" -ErrorAction SilentlyContinue |
    Where-Object {
      $_.ExecutablePath -and ([IO.Path]::GetFullPath($_.ExecutablePath).StartsWith($here, [StringComparison]::OrdinalIgnoreCase))
    }
}

function Send-ConsoleText([int]$PidTarget, [string]$Text) {
  # Optional reuse of Trinity helper if present on this machine
  $trinityHelper = '<SERVER_ROOT>\_tools\restart-worldserver.ps1'
  if (Test-Path $trinityHelper) {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $trinityHelper -SendConsolePid $PidTarget -SendConsoleText $Text
    return $LASTEXITCODE
  }
  return 1
}

function Wait-Exit([int]$ProcId, [int]$Seconds) {
  $deadline = (Get-Date).AddSeconds($Seconds)
  while ((Get-Date) -lt $deadline) {
    if (-not (Get-Process -Id $ProcId -ErrorAction SilentlyContinue)) { return $true }
    Start-Sleep -Seconds 2
  }
  return -not [bool](Get-Process -Id $ProcId -ErrorAction SilentlyContinue)
}

$scoped = @(Get-Scoped)
if ($scoped.Count -eq 0) {
  Write-Host "No bnetserver/worldserver under $here"
  exit 0
}
Write-Host "IT: Arresto graduale path-scoped sotto $here"
Write-Host "EN: Graceful path-scoped stop under $here"
Write-Host ("Targets: " + (($scoped | ForEach-Object { "$($_.Name):$($_.ProcessId)" }) -join ', '))
if ($DryRun) {
  Write-Host "DRYRUN: would announce IT/EN + server shutdown $ShutdownDelay to worldserver, wait $ExitTimeoutSec s, then force those PIDs only. Countdown ${ShutdownDelay}s IT+EN retained."
  exit 0
}

$world = @($scoped | Where-Object { $_.Name -eq 'worldserver.exe' })
foreach ($w in $world) {
  $annIT = "Riavvio del server tra $ShutdownDelay secondi. Salva e disconnettiti in sicurezza."
  $annEN = "Server restart in $ShutdownDelay seconds. Please save and log out safely."
  [void](Send-ConsoleText $w.ProcessId "announce $annIT")
  Start-Sleep -Milliseconds 400
  [void](Send-ConsoleText $w.ProcessId "announce $annEN")
  Start-Sleep -Milliseconds 400
  $rc = Send-ConsoleText $w.ProcessId "server shutdown $ShutdownDelay"
  Write-Host "Sent shutdown to worldserver PID $($w.ProcessId) (send_rc=$rc)"
}

for ($s = $ShutdownDelay; $s -ge 1; $s--) {
  $still = @(Get-Scoped | Where-Object { $_.Name -eq 'worldserver.exe' })
  if ($still.Count -eq 0) { Write-Host "worldserver exited during countdown"; break }
  Write-Host "IT/EN countdown: ${s}s remaining - world still: $(($still | ForEach-Object ProcessId) -join ',')"
  Start-Sleep -Seconds 1
}

foreach ($w in @(Get-Scoped | Where-Object { $_.Name -eq 'worldserver.exe' })) {
  Write-Host "Waiting exit worldserver $($w.ProcessId) max $ExitTimeoutSec s..."
  if (-not (Wait-Exit $w.ProcessId $ExitTimeoutSec)) {
    Write-Warning "Force-stopping worldserver PID $($w.ProcessId) only"
    Stop-Process -Id $w.ProcessId -Force -ErrorAction SilentlyContinue
    [void](Wait-Exit $w.ProcessId 60)
  }
}

foreach ($prefix in $WindowTitlePrefixes) {
  cmd /c "taskkill /FI `"WINDOWTITLE eq $prefix*`" >nul 2>&1"
}
Start-Sleep -Seconds 3
foreach ($b in @(Get-Scoped | Where-Object { $_.Name -eq 'bnetserver.exe' })) {
  cmd /c "taskkill /PID $($b.ProcessId) >nul 2>&1"
}
$deadline = (Get-Date).AddSeconds(30)
while ((Get-Date) -lt $deadline) {
  if (@(Get-Scoped).Count -eq 0) { break }
  Start-Sleep -Seconds 2
}
foreach ($p in @(Get-Scoped)) {
  Write-Warning "Force-stopping leftover PID $($p.ProcessId) path=$($p.ExecutablePath)"
  Stop-Process -Id $p.ProcessId -Force -ErrorAction SilentlyContinue
}
foreach ($prefix in $WindowTitlePrefixes) {
  cmd /c "taskkill /F /FI `"WINDOWTITLE eq $prefix*`" >nul 2>&1"
}
$left = @(Get-Scoped)
if ($left.Count -gt 0) {
  Write-Error ("Still running: " + (($left | ForEach-Object { "$($_.ProcessId):$($_.ExecutablePath)" }) -join ', '))
  exit 1
}
Write-Host "Stopped all scoped processes under $here"
exit 0
