# Liga/desliga a luz do botao Home do Pro Controller (subcomando 0x38) pelo HID do dongle.
# Uso: powershell -File tools\home-light.ps1 -State on|off|pulse
param([ValidateSet('on', 'off', 'pulse')][string]$State = 'on')

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices; using Microsoft.Win32.SafeHandles;
public static class HidHome {
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern SafeFileHandle CreateFile(string f, uint a, uint s, IntPtr sa, uint d, uint fl, IntPtr t);
  [DllImport("kernel32.dll", SetLastError=true)]
  public static extern bool WriteFile(SafeFileHandle h, byte[] b, uint n, out uint w, IntPtr o);
}
'@

$id = (Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match '^HID\\VID_057E&PID_2009&MI_00' } | Select-Object -First 1).InstanceId
if (-not $id) { Write-Host "Dongle nao encontrado"; exit 1 }
$path = '\\?\' + ($id -replace '\\','#') + '#{4d1e55b2-f16f-11cf-88cb-001111000030}'
$h = [HidHome]::CreateFile($path, [uint32]3221225472, [uint32]3, [IntPtr]::Zero, [uint32]3, [uint32]0, [IntPtr]::Zero)
if ($h.IsInvalid) { Write-Host "CreateFile falhou"; exit 1 }

# argumentos do 0x38: [0] = (mini ciclos << 4) | duracao global; [1] = (intensidade inicial << 4) | ciclos completos (0 = infinito)
$args38 = switch ($State) {
  'on'    { [byte[]](0x01, 0xF0, 0xF0) }      # fixo, 100%
  'off'   { [byte[]](0x01, 0x00, 0x00) }      # apagado
  'pulse' { [byte[]](0x2F, 0x00, 0xF0, 0x11, 0x11) }   # 2 mini ciclos de 175 ms: 100% e 0%, repete
}
$b = New-Object byte[] 64
$b[0] = 0x01; $b[1] = 0x00                    # relatorio 0x01, contador
$b[2] = 0x00; $b[3] = 0x01; $b[4] = 0x40; $b[5] = 0x40; $b[6] = 0x00; $b[7] = 0x01; $b[8] = 0x40; $b[9] = 0x40   # vibracao neutra
$b[10] = 0x38
for ($i = 0; $i -lt $args38.Length; $i++) { $b[11 + $i] = $args38[$i] }
$w = [uint32]0
$ok = [HidHome]::WriteFile($h, $b, [uint32]64, [ref]$w, [IntPtr]::Zero)
Write-Host "luz do Home: $State (escrita ok=$ok, $w bytes)"
$h.Close()
