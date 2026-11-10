param([Parameter(Mandatory=$true)][string]$RuntimeDir)
$here = [IO.Path]::GetFullPath($RuntimeDir).TrimEnd('\')
$p = @(Get-Process -Name bnetserver,worldserver -ErrorAction SilentlyContinue |
  Where-Object { $_.Path -and ([IO.Path]::GetFullPath($_.Path).StartsWith($here, [StringComparison]::OrdinalIgnoreCase)) })
if ($p.Count -gt 0) {
  Write-Host ("Already running under ${here}: " + (($p | ForEach-Object { "$($_.ProcessName)=$($_.Id)" }) -join ', '))
  exit 0
}
exit 1
