<#
.SYNOPSIS
  Aeternum Admin API token wizard: Generate / Rotate / Recover (per-instance).
.DESCRIPTION
  - CSPRNG via System.Security.Cryptography.RandomNumberGenerator (32 bytes -> base64url ~43 chars).
  - Token shown ONCE to the console; never written to log files, never placed on command lines.
  - Saved to Windows User env (HKCU) using the env var name from the instance conf
    (Admin.TokenFromEnv; empty/missing -> AETERNUM_ADMIN_TOKEN).
  - Operator token is OPTIONAL (-AlsoOperator).
  - Rotate/Recover restart the given instance ONLY when -ConfirmRestart is passed
    (path-scoped Stop + START_SERVER/START_PUBLIC helpers). Trinity is never touched.
  - DryRun/WhatIf: plan only; no env write, no restart.
.NOTES
  Auth verification uses GET /metrics (Bearer) because GET /health is anonymous by design.
  /health is still probed for liveness (expect 200).
#>
[CmdletBinding(SupportsShouldProcess=$true)]
param(
  [Parameter(Mandatory=$true)]
  [ValidateSet('Generate','Rotate','Recover')]
  [string]$Action,

  [Parameter(Mandatory=$true)]
  [string]$RuntimeDir,

  [string]$ConfFile = '',
  [string]$StartBat = '',
  [string]$StopHelper = '',
  [string]$AdminPort = '9300',
  [switch]$AlsoOperator,
  [switch]$ConfirmRestart,
  [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$WhatIfPreference = $WhatIfPreference -or $DryRun

function Write-MgmtLog([string]$Instance, [string]$Act, [string]$Result, [string]$Detail = '') {
  $logDir = Join-Path $Instance 'logs'
  if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir -Force -WhatIf:$false | Out-Null }
  $log = Join-Path $logDir 'admin_token_mgmt.log'
  $ts = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
  $line = "$ts instance=$Instance action=$Act result=$Result"
  if ($Detail) { $line += " detail=$Detail" }
  # Never append secrets
  Add-Content -Path $log -Value $line -Encoding UTF8 -WhatIf:$false
}

function Get-ConfValue([string]$Path, [string]$Key) {
  if (-not (Test-Path $Path)) { return $null }
  $re = '^\s*' + [regex]::Escape($Key) + '\s*=\s*"?([^"#\r\n]+?)"?\s*(#.*)?$'
  foreach ($line in Get-Content -Path $Path -ErrorAction Stop) {
    $m = [regex]::Match($line, $re)
    if ($m.Success) { return $m.Groups[1].Value.Trim() }
  }
  return $null
}

function New-AdminTokenString {
  $bytes = New-Object byte[] 32
  [System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
  $b64 = [Convert]::ToBase64String($bytes)
  return ($b64.TrimEnd('=') -replace '\+','-' -replace '/','_')
}

function ConvertTo-SecureStringPlain([string]$Plain) {
  $ss = New-Object System.Security.SecureString
  foreach ($ch in $Plain.ToCharArray()) { $ss.AppendChar($ch) }
  $ss.MakeReadOnly()
  return $ss
}

function Get-PlainFromSecure([System.Security.SecureString]$Secure) {
  $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($Secure)
  try { return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr) }
  finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr) }
}

function Set-UserEnv([string]$Name, [System.Security.SecureString]$Secure) {
  $plain = Get-PlainFromSecure $Secure
  try {
    [Environment]::SetEnvironmentVariable($Name, $plain, 'User')
    Set-Item -Path "Env:$Name" -Value $plain
  } finally {
    $plain = $null
    [GC]::Collect()
  }
}

function Invoke-AdminRequest([string]$Url, [System.Security.SecureString]$TokenSecure) {
  $plain = $null
  try {
    if ($TokenSecure) { $plain = Get-PlainFromSecure $TokenSecure }
    $headers = @{}
    if ($plain) { $headers['Authorization'] = "Bearer $plain" }
    try {
      $resp = Invoke-WebRequest -Uri $Url -Headers $headers -Method GET -UseBasicParsing -TimeoutSec 10
      return [pscustomobject]@{ Status = [int]$resp.StatusCode; Body = $resp.Content }
    } catch {
      $code = -1
      if ($_.Exception.Response) { $code = [int]$_.Exception.Response.StatusCode }
      return [pscustomobject]@{ Status = $code; Body = '' }
    }
  } finally {
    $plain = $null
    if ($headers) { $headers.Clear() }
    [GC]::Collect()
  }
}

$RuntimeDir = [IO.Path]::GetFullPath($RuntimeDir).TrimEnd('\')
if (-not (Test-Path $RuntimeDir)) { throw "RuntimeDir not found: $RuntimeDir" }

# Resolve conf (worldserver.conf): runtime dir, or ../conf for _public_iem1 layout
if (-not $ConfFile) {
  $cands = @(
    (Join-Path $RuntimeDir 'worldserver.conf'),
    (Join-Path $RuntimeDir '..\conf\worldserver.conf')
  )
  foreach ($c in $cands) {
    if (Test-Path $c) { $ConfFile = [IO.Path]::GetFullPath($c); break }
  }
}
if (-not $ConfFile -or -not (Test-Path $ConfFile)) {
  throw "worldserver.conf not found near $RuntimeDir (pass -ConfFile)"
}

$envName = Get-ConfValue $ConfFile 'Admin.TokenFromEnv'
if ([string]::IsNullOrWhiteSpace($envName)) { $envName = 'AETERNUM_ADMIN_TOKEN' }
$opEnvName = Get-ConfValue $ConfFile 'Admin.OperatorTokenFromEnv'
if ([string]::IsNullOrWhiteSpace($opEnvName)) { $opEnvName = 'AETERNUM_ADMIN_OPERATOR_TOKEN' }
$minLen = 32
$minConf = Get-ConfValue $ConfFile 'Admin.TokenMinLength'
if ($minConf -as [int]) { $minLen = [Math]::Max(32, [int]$minConf) }

if (-not $StopHelper) {
  $localStop = Join-Path $RuntimeDir 'Stop-AeternumPathScoped.ps1'
  $toolsStop = '<ENGINE_ROOT>\_tools\Stop-AeternumPathScoped.ps1'
  if (Test-Path $localStop) { $StopHelper = $localStop }
  elseif (Test-Path $toolsStop) { $StopHelper = $toolsStop }
}
if (-not $StartBat) {
  foreach ($n in @('START_SERVER.bat','START_PUBLIC.bat')) {
    $p = Join-Path (Split-Path $RuntimeDir -Parent) $n
    if (Test-Path (Join-Path $RuntimeDir $n)) { $StartBat = Join-Path $RuntimeDir $n; break }
    if (Test-Path $p) { $StartBat = $p; break }
  }
  if (-not $StartBat -and (Test-Path (Join-Path $RuntimeDir 'START_SERVER.bat'))) {
    $StartBat = Join-Path $RuntimeDir 'START_SERVER.bat'
  }
}

Write-Host "=== Aeternum Admin Token: $Action ==="
Write-Host "RuntimeDir : $RuntimeDir"
Write-Host "ConfFile   : $ConfFile"
Write-Host "Env var    : $envName (User scope)"
Write-Host "Op env var : $opEnvName (optional)"
Write-Host "MinLength  : $minLen"
Write-Host "DryRun     : $WhatIfPreference"
Write-Host "ConfirmRestart: $ConfirmRestart"

# Capture old token (SecureString) for post-rotate verify — never print
$oldSecure = $null
$oldPlainProbe = [Environment]::GetEnvironmentVariable($envName, 'User')
if ($oldPlainProbe) {
  if ($oldPlainProbe.Length -ge $minLen) { $oldSecure = ConvertTo-SecureStringPlain $oldPlainProbe }
  $oldPlainProbe = $null
  [GC]::Collect()
}

if ($Action -eq 'Generate' -or $Action -eq 'Rotate' -or $Action -eq 'Recover') {
  if ($WhatIfPreference) {
    Write-Host "WhatIf/DryRun: would generate CSPRNG token (len>=$minLen), set User env $envName; token NOT shown in DryRun; no restart."
    Write-MgmtLog $RuntimeDir $Action 'dryrun' "env=$envName"
    exit 0
  }

  $newPlain = New-AdminTokenString
  if ($newPlain.Length -lt $minLen) { throw "Generated token shorter than $minLen (unexpected)" }
  $newSecure = ConvertTo-SecureStringPlain $newPlain

  Write-Host ''
  Write-Host '===== NEW TOKEN (shown ONCE - copy now; will not be logged) ====='
  Write-Host $newPlain
  Write-Host '===== END TOKEN ====='
  Write-Host ''
  $newPlain = $null
  [GC]::Collect()

  $opSecure = $null
  if ($AlsoOperator) {
    $opPlain = New-AdminTokenString
    $opSecure = ConvertTo-SecureStringPlain $opPlain
    Write-Host '===== NEW OPERATOR TOKEN (shown ONCE) ====='
    Write-Host $opPlain
    Write-Host '===== END OPERATOR TOKEN ====='
    $opPlain = $null
    [GC]::Collect()
  }


  if ($PSCmdlet.ShouldProcess("User env $envName", "Set token")) {
    Set-UserEnv -Name $envName -Secure $newSecure
    if ($AlsoOperator -and $opSecure) { Set-UserEnv -Name $opEnvName -Secure $opSecure }
    Write-Host "Saved User env var name=$envName present=$([bool][Environment]::GetEnvironmentVariable($envName,'User')) len=$([Environment]::GetEnvironmentVariable($envName,'User').Length)"
    # Re-read length only — do not print value (length print is OK)
  }

  $needsRestart = ($Action -eq 'Rotate' -or $Action -eq 'Recover')
  if ($needsRestart) {
    if (-not $ConfirmRestart) {
      Write-Host 'Token saved. Pass -ConfirmRestart to path-scoped stop/start THIS instance only.'
      Write-MgmtLog $RuntimeDir $Action 'saved_no_restart' "env=$envName"
      exit 0
    }
    if (-not $StopHelper -or -not (Test-Path $StopHelper)) { throw "Stop helper missing" }
    if (-not $StartBat -or -not (Test-Path $StartBat)) { throw "Start bat missing near runtime" }

    Write-Host "Stopping path-scoped under $RuntimeDir (120s IT+EN)..."
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $StopHelper -RuntimeDir $RuntimeDir -ShutdownDelay 120 -ExitTimeoutSec 180
    if ($LASTEXITCODE -ne 0) {
      Write-MgmtLog $RuntimeDir $Action 'stop_failed' "rc=$LASTEXITCODE"
      throw "Stop failed rc=$LASTEXITCODE"
    }

    # Start with env injected into child (fresh User value)
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = 'cmd.exe'
    $psi.Arguments = "/c `"$StartBat`""
    $psi.WorkingDirectory = (Split-Path $StartBat -Parent)
    $psi.UseShellExecute = $false
    $tok = [Environment]::GetEnvironmentVariable($envName, 'User')
    if ($tok) { $psi.EnvironmentVariables[$envName] = $tok; $tok = $null }
    $op = [Environment]::GetEnvironmentVariable($opEnvName, 'User')
    if ($op) { $psi.EnvironmentVariables[$opEnvName] = $op; $op = $null }
    [GC]::Collect()
    $p = [Diagnostics.Process]::Start($psi)
    Write-Host "Start launched pid=$($p.Id)"

    # Wait admin port
    $deadline = (Get-Date).AddSeconds(180)
    while ((Get-Date) -lt $deadline) {
      if (Get-NetTCPConnection -LocalPort ([int]$AdminPort) -State Listen -ErrorAction SilentlyContinue) { break }
      Start-Sleep -Seconds 3
    }

    $health = Invoke-AdminRequest "http://127.0.0.1:$AdminPort/health" $null
    $newOk = Invoke-AdminRequest "http://127.0.0.1:$AdminPort/metrics" $newSecure
    $oldStat = -1
    if ($oldSecure) { $oldStat = (Invoke-AdminRequest "http://127.0.0.1:$AdminPort/metrics" $oldSecure).Status }

    Write-Host "Verify /health status=$($health.Status) (expect 200, anonymous)"
    Write-Host "Verify /metrics new token status=$($newOk.Status) (expect 200)"
    Write-Host "Verify /metrics old token status=$oldStat (expect 401/403 if old existed)"

    $pass = ($health.Status -eq 200) -and ($newOk.Status -eq 200) -and (($oldStat -lt 0) -or ($oldStat -eq 401) -or ($oldStat -eq 403))
    Write-MgmtLog $RuntimeDir $Action $(if ($pass) { 'ok' } else { 'verify_failed' }) "health=$($health.Status);new=$($newOk.Status);old=$oldStat;env=$envName"
    if (-not $pass) { throw "Post-restart verify failed (see statuses above; token values never logged)" }
    Write-Host 'Rotation/Recover verify PASS'
  } else {
    Write-MgmtLog $RuntimeDir $Action 'generated' "env=$envName"
  }
}

exit 0