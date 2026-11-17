# Grava o firmware. Se o dongle ja roda este firmware, reinicia em BOOTSEL abrindo a serial a 1200 baud.
# Uso: powershell -File tools\flash.ps1 [-Release] [-Uf2 caminho]
# O build release nao tem a serial: a gravacao seguinte exige BOOTSEL manual (ou um build debug).
param([string]$Uf2, [switch]$Release)
if (-not $Uf2) {
  $dir = if ($Release) { 'build-release' } else { 'build' }
  $Uf2 = "$PSScriptRoot\..\firmware\$dir\procontroller_dongle.uf2"
}

function Find-Boot { Get-Volume | Where-Object { $_.FileSystemLabel -match 'RP2' } | Select-Object -First 1 }

$v = Find-Boot
if (-not $v) {
  $p = Get-CimInstance Win32_PnPEntity | Where-Object { $_.DeviceID -match 'VID_057E&PID_2009' -and $_.Name -match '\(COM\d+\)' } |
       ForEach-Object { [regex]::Match($_.Name,'COM\d+').Value } | Select-Object -First 1
  if ($p) {
    Write-Host "Reiniciando $p em BOOTSEL (1200 baud)..."
    try { $sp = New-Object System.IO.Ports.SerialPort $p,1200; $sp.Open(); Start-Sleep -Milliseconds 200; $sp.Close() } catch {}
    for ($i = 0; $i -lt 30 -and -not $v; $i++) { Start-Sleep -Milliseconds 300; $v = Find-Boot }
  }
}
if (-not $v) { Write-Host "Unidade RP2350 nao encontrada. Segure BOOTSEL e plugue o Pico."; exit 1 }
Copy-Item $Uf2 "$($v.DriveLetter):\"
Write-Host "Gravado em $($v.DriveLetter):"
