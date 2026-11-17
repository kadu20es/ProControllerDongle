# Mede, no PC, a cadencia dos relatorios 0x30 do dongle: intervalos entre relatorios (jitter) e relatorios perdidos.
# Os perdidos sao detectados pelo contador de tempo do proprio controle (byte 1 do relatorio, ~5 ms por tick).
# Fecha Steam/emuladores antes. Uso: powershell -File tools\latency-host.ps1 [-Seconds 30]
param([int]$Seconds = 30)

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices; using Microsoft.Win32.SafeHandles;
public static class HidLat {
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern SafeFileHandle CreateFile(string f, uint a, uint s, IntPtr sa, uint d, uint fl, IntPtr t);
  [DllImport("kernel32.dll", SetLastError=true)]
  public static extern bool WriteFile(SafeFileHandle h, byte[] b, uint n, out uint w, IntPtr o);
  [DllImport("kernel32.dll", SetLastError=true)]
  public static extern bool ReadFile(SafeFileHandle h, byte[] b, uint n, out uint r, IntPtr o);
}
'@

$id = (Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match '^HID\\VID_057E&PID_2009&MI_00' } | Select-Object -First 1).InstanceId
if (-not $id) { Write-Host "Dongle nao encontrado"; exit 1 }
$path = '\\?\' + ($id -replace '\\','#') + '#{4d1e55b2-f16f-11cf-88cb-001111000030}'
$h = [HidLat]::CreateFile($path, [uint32]3221225472, [uint32]3, [IntPtr]::Zero, [uint32]3, [uint32]0, [IntPtr]::Zero)
if ($h.IsInvalid) { Write-Host "CreateFile falhou"; exit 1 }

# prioridade alta para o proprio script atrapalhar menos a medicao
try { (Get-Process -Id $PID).PriorityClass = 'High' } catch {}

$cmd = New-Object byte[] 64; $cmd[0] = 0x80; $cmd[1] = 0x04; $w = [uint32]0
[void][HidLat]::WriteFile($h, $cmd, [uint32]64, [ref]$w, [IntPtr]::Zero)   # liga o fluxo 0x30

$buf = New-Object byte[] 64
$freq = [double][Diagnostics.Stopwatch]::Frequency
$intervals = New-Object 'System.Collections.Generic.List[double]'
$lost = 0; $n = 0; $prevT = 0.0; $prevTimer = -1
$t0 = [Diagnostics.Stopwatch]::GetTimestamp()
Write-Host "Medindo $Seconds s (mexa o controle normalmente)..."
while (([Diagnostics.Stopwatch]::GetTimestamp() - $t0) / $freq -lt $Seconds) {
  $r = [uint32]0
  if (-not [HidLat]::ReadFile($h, $buf, [uint32]64, [ref]$r, [IntPtr]::Zero)) { break }
  if ($buf[0] -ne 0x30) { continue }
  $t = [Diagnostics.Stopwatch]::GetTimestamp() / $freq * 1000.0       # ms
  if ($n -gt 0) { $intervals.Add($t - $prevT) }
  $prevT = $t; $n++
  $timer = [int]$buf[1]
  if ($prevTimer -ge 0) {
    $d = ($timer - $prevTimer + 256) % 256
    if ($d -ge 5) { $lost += [math]::Max(0, [math]::Round($d / 3.0) - 1) }   # ~3 ticks por relatorio; mais que isso = perdeu
  }
  $prevTimer = $timer
}
$h.Close()

if ($intervals.Count -lt 10) { Write-Host "Poucos relatorios ($n). O controle esta conectado?"; exit 1 }
$sorted = $intervals.ToArray(); [Array]::Sort($sorted)
function P($p) { $sorted[[math]::Min($sorted.Length - 1, [int][math]::Floor($p / 100.0 * $sorted.Length))] }
$mean = ($sorted | Measure-Object -Average).Average
$dur = $sorted | Measure-Object -Sum
Write-Host ("Relatorios 0x30: {0} em {1:N1} s  ->  {2:N1} por segundo" -f $n, ($dur.Sum / 1000.0), ($n / ($dur.Sum / 1000.0)))
Write-Host ("Intervalo entre relatorios (ms): media {0:N2}  mediana {1:N2}  p95 {2:N2}  p99 {3:N2}  max {4:N2}" -f $mean, (P 50), (P 95), (P 99), $sorted[-1])
Write-Host ("Intervalos >= 25 ms: {0}   >= 40 ms: {1}   (>= 100 ms: {2})" -f ($sorted | Where-Object { $_ -ge 25 }).Count, ($sorted | Where-Object { $_ -ge 40 }).Count, ($sorted | Where-Object { $_ -ge 100 }).Count)
Write-Host ("Relatorios perdidos (pelo contador do controle): {0}  ({1:N2}% dos {2})" -f $lost, (100.0 * $lost / ($n + $lost)), ($n + $lost))
Write-Host "Nota: o jitter inclui o escalonamento do Windows/PowerShell; trate p99/max como limite superior."
