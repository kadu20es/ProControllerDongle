# Mede o IMU em 3 posicoes (5 s cada) lendo os relatorios 0x30 do dongle direto pelo HID.
# Sinais sonoros: 1 beep = comece a fase 1 (mesa); 2 beeps = fase 2 (duas maos, "reta"); 3 beeps = fase 3 (bandeja); beep longo = fim.
# Uso: powershell -File tools\imu-phases.ps1
param([int]$Prep = 4, [int]$Phase = 5, [int]$Gap = 4)

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices; using Microsoft.Win32.SafeHandles;
public static class HidIo2 {
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
$h = [HidIo2]::CreateFile($path, [uint32]3221225472, [uint32]3, [IntPtr]::Zero, [uint32]3, [uint32]0, [IntPtr]::Zero)
if ($h.IsInvalid) { Write-Host "CreateFile falhou"; exit 1 }
$cmd = New-Object byte[] 64; $cmd[0] = 0x80; $cmd[1] = 0x04; $w = [uint32]0
[void][HidIo2]::WriteFile($h, $cmd, [uint32]64, [ref]$w, [IntPtr]::Zero)

function S16($b, $i) { $v = [int]$b[$i] + 256 * [int]$b[$i+1]; if ($v -ge 32768) { $v - 65536 } else { $v } }
function Beep($n) { for ($i = 0; $i -lt $n; $i++) { [console]::Beep(988, 180); Start-Sleep -Milliseconds 120 } }

# origem de fabrica do acelerometro deste controle (SPI 0x6020): x=-182 y=-157 z=436
$org = @(-182, -157, 436)

$names = 'mesa', 'duas maos "reta"', 'bandeja'
$res = @()
$buf = New-Object byte[] 64
$t0 = Get-Date
$schedule = @()   # (inicio, fim) de cada fase, em segundos
for ($p = 0; $p -lt 3; $p++) { $s = $Prep + $p * ($Phase + $Gap); $schedule += ,@($s, ($s + $Phase)) }
$cur = -1; $beeped = @($false, $false, $false)
$sum = @(); for ($p = 0; $p -lt 3; $p++) { $sum += ,(New-Object double[] 6) }
$cnt = @(0, 0, 0)
$endAt = $schedule[2][1]
$beepNext = -1
$reads = 0
$why = 'fim do tempo'
while ($true) {
  $t = ((Get-Date) - $t0).TotalSeconds
  if ($t -ge $endAt) { break }
  $r = [uint32]0
  if (-not [HidIo2]::ReadFile($h, $buf, [uint32]64, [ref]$r, [IntPtr]::Zero)) { $why = 'ReadFile falhou'; break }
  if ($buf[0] -ne 0x30) { continue }
  $reads++
  for ($p = 0; $p -lt 3; $p++) {
    if (-not $beeped[$p] -and $t -ge $schedule[$p][0]) { $beeped[$p] = $true; Beep ($p + 1) }
  }
  $t = ((Get-Date) - $t0).TotalSeconds
  for ($p = 0; $p -lt 3; $p++) {
    if ($t -ge ($schedule[$p][0] + 0.6) -and $t -lt $schedule[$p][1]) {     # ignora 0,6 s apos o beep
      for ($s = 0; $s -lt 3; $s++) {
        $o = 13 + 12 * $s
        for ($k = 0; $k -lt 6; $k++) { $sum[$p][$k] += (S16 $buf ($o + 2 * $k)) }
        $cnt[$p]++
      }
    }
  }
}
[console]::Beep(660, 700)
$h.Close()
Write-Host ("duracao {0:N1} s, relatorios 0x30: {1}, motivo do fim: {2}" -f ((Get-Date) - $t0).TotalSeconds, $reads, $why)

function Deg($rad) { [math]::Round($rad * 180 / [math]::PI, 1) }
for ($p = 0; $p -lt 3; $p++) {
  if ($cnt[$p] -eq 0) { Write-Host ("{0}: sem amostras" -f $names[$p]); continue }
  $m = @(); for ($k = 0; $k -lt 6; $k++) { $m += $sum[$p][$k] / $cnt[$p] }
  $ax = $m[0]; $ay = $m[1]; $az = $m[2]
  $cx = $ax - $org[0]; $cy = $ay - $org[1]; $cz = $az - $org[2]
  $roll  = Deg ([math]::Atan2($ay, $az));  $pitch  = Deg ([math]::Atan2(-$ax, [math]::Sqrt($ay*$ay + $az*$az)))
  $rollc = Deg ([math]::Atan2($cy, $cz));  $pitchc = Deg ([math]::Atan2(-$cx, [math]::Sqrt($cy*$cy + $cz*$cz)))
  Write-Host ("{0}: amostras {1}" -f $names[$p], $cnt[$p])
  Write-Host ("   acc medio  X {0,7:N0}  Y {1,7:N0}  Z {2,7:N0}   | gyro medio  X {3,6:N0}  Y {4,6:N0}  Z {5,6:N0}" -f $ax, $ay, $az, $m[3], $m[4], $m[5])
  Write-Host ("   inclinacao (bruta):     roll {0,6} graus  pitch {1,6} graus" -f $roll, $pitch)
  Write-Host ("   inclinacao (c/ origem): roll {0,6} graus  pitch {1,6} graus" -f $rollc, $pitchc)
}
