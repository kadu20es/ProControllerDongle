# Le relatorios 0x30 do dongle direto pelo HID do Windows e mostra, por segundo, a atividade do IMU.
# Fecha Steam/emuladores antes. Uso: powershell -File tools\imu-test.ps1 [-Seconds 16]
param([int]$Seconds = 16)

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices; using Microsoft.Win32.SafeHandles;
public static class HidIo {
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern SafeFileHandle CreateFile(string f, uint a, uint s, IntPtr sa, uint d, uint fl, IntPtr t);
  [DllImport("kernel32.dll", SetLastError=true)]
  public static extern bool WriteFile(SafeFileHandle h, byte[] b, uint n, out uint w, IntPtr o);
  [DllImport("kernel32.dll", SetLastError=true)]
  public static extern bool ReadFile(SafeFileHandle h, byte[] b, uint n, out uint r, IntPtr o);
}
'@

$id = (Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match '^HID\\VID_057E&PID_2009' } | Select-Object -First 1).InstanceId
if (-not $id) { Write-Host "Dongle nao encontrado"; exit 1 }
$path = '\\?\' + ($id -replace '\\','#') + '#{4d1e55b2-f16f-11cf-88cb-001111000030}'
$h = [HidIo]::CreateFile($path, [uint32]3221225472, [uint32]3, [IntPtr]::Zero, [uint32]3, [uint32]0, [IntPtr]::Zero)
if ($h.IsInvalid) { Write-Host "CreateFile falhou: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())"; exit 1 }

# liga o fluxo 0x30 (como o Steam/SDL fazem): 0x80 0x04
$cmd = New-Object byte[] 64; $cmd[0] = 0x80; $cmd[1] = 0x04; $w = [uint32]0
[void][HidIo]::WriteFile($h, $cmd, [uint32]64, [ref]$w, [IntPtr]::Zero)

function S16($b, $i) { $v = [int]$b[$i] + 256 * [int]$b[$i+1]; if ($v -ge 32768) { $v - 65536 } else { $v } }
$buf = New-Object byte[] 64
$t0 = Get-Date
Write-Host "seg | relatorios | max|gyro| (x,y,z)        | acc Z (min..max)"
$sec = 0; $n = 0; $gmax = @(0,0,0); $zmin = 32767; $zmax = -32768
while ($sec -lt $Seconds) {
  $r = [uint32]0
  if (-not [HidIo]::ReadFile($h, $buf, [uint32]64, [ref]$r, [IntPtr]::Zero)) { break }
  if ($buf[0] -ne 0x30) { continue }
  $n++
  for ($s = 0; $s -lt 3; $s++) {
    $o = 13 + 12 * $s
    for ($k = 0; $k -lt 3; $k++) { $g = [math]::Abs((S16 $buf ($o + 6 + 2*$k))); if ($g -gt $gmax[$k]) { $gmax[$k] = $g } }
    $az = S16 $buf ($o + 4); if ($az -lt $zmin) { $zmin = $az }; if ($az -gt $zmax) { $zmax = $az }
  }
  $now = [int]((Get-Date) - $t0).TotalSeconds
  if ($now -gt $sec) {
    Write-Host ("{0,3} | {1,10} | {2,5} {3,5} {4,5}        | {5,6}..{6,6}" -f ($sec + 1), $n, $gmax[0], $gmax[1], $gmax[2], $zmin, $zmax)
    $sec = $now; $n = 0; $gmax = @(0,0,0); $zmin = 32767; $zmax = -32768
  }
}
$h.Close()
