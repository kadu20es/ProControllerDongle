# Le o log serial do dongle (Pico, VID 2E8A) e reconecta se a porta sumir.
# Uso: powershell -File tools\serial-log.ps1 [-Seconds 60] [-Port COMx]
param([int]$Seconds = 60, [string]$Port)
function Find-Port {
  Get-CimInstance Win32_PnPEntity | Where-Object { $_.DeviceID -match 'VID_057E&PID_2009' -and $_.Name -match '\(COM\d+\)' } |
    ForEach-Object { [regex]::Match($_.Name,'COM\d+').Value } | Select-Object -First 1
}
$end = (Get-Date).AddSeconds($Seconds)
$sp = $null
while ((Get-Date) -lt $end) {
  try {
    if (-not $sp -or -not $sp.IsOpen) {
      $p = if ($Port) { $Port } else { Find-Port }
      if (-not $p) { Start-Sleep -Milliseconds 300; continue }
      $sp = New-Object System.IO.Ports.SerialPort $p,115200
      $sp.DtrEnable = $true; $sp.Open(); Write-Host "[porta $p aberta]"
    }
    if ($sp.BytesToRead) { Write-Host -NoNewline $sp.ReadExisting() } else { Start-Sleep -Milliseconds 50 }
  } catch { try { $sp.Dispose() } catch {}; $sp = $null; Start-Sleep -Milliseconds 300 }
}
