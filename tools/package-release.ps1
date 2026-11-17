# Gera os arquivos de release em dist\ (firmware release e debug, somas SHA-256 e texto da pagina do release).
# Uso: powershell -File tools\package-release.ps1 [-NoBuild]
param([switch]$NoBuild)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
$cmake = Get-Content "$root\firmware\CMakeLists.txt" -Raw
if ($cmake -notmatch 'set\(PROCON_VERSION "([^"]+)"\)') { throw "PROCON_VERSION nao encontrada em firmware\CMakeLists.txt" }
$ver = $Matches[1]
$tag = "v$ver"

if (-not $NoBuild) {
  Write-Host "Compilando release e debug (versao $ver)..."
  & cmd /c "`"$PSScriptRoot\build.cmd`" release clean" | Out-Null
  if ($LASTEXITCODE -ne 0) { throw "falha ao compilar o release" }
  & cmd /c "`"$PSScriptRoot\build.cmd`" clean" | Out-Null
  if ($LASTEXITCODE -ne 0) { throw "falha ao compilar o debug" }
}

$relUf2 = "$root\firmware\build-release\procontroller_dongle.uf2"
$dbgUf2 = "$root\firmware\build\procontroller_dongle.uf2"
foreach ($f in $relUf2, $dbgUf2) { if (-not (Test-Path $f)) { throw "arquivo nao encontrado: $f" } }

$dist = "$root\dist"
if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }
New-Item -ItemType Directory $dist | Out-Null
$relName = "ProControllerDongle-$tag.uf2"
$dbgName = "ProControllerDongle-$tag-debug.uf2"
Copy-Item $relUf2 "$dist\$relName"
Copy-Item $dbgUf2 "$dist\$dbgName"

$sums = foreach ($n in $relName, $dbgName) {
  $h = (Get-FileHash "$dist\$n" -Algorithm SHA256).Hash.ToLower()
  "$h  $n"
}
$sums | Set-Content "$dist\SHA256SUMS.txt" -Encoding ascii

$notes = @"
# Pro Controller Dongle $tag

Receptor USB para o **Nintendo Switch Pro Controller** com uma **Raspberry Pi Pico 2 W**: o Pico conversa com o controle
por Bluetooth e aparece no PC como um Pro Controller USB nativo (057E:2009).

> **Requer uma Raspberry Pi Pico 2 W** (RP2350 + Bluetooth). A Pico 2 sem "W" nao tem Bluetooth e nao funciona.

## Arquivos
| Arquivo | Para que serve |
|---|---|
| ``$relName`` | **Firmware release** (uso diario, sem porta serial) |
| ``$dbgName`` | Firmware debug (porta serial de log e comandos de teste) |
| ``SHA256SUMS.txt`` | Somas de verificacao |

## Instalacao
1. Segure o botao **BOOTSEL** da Pico, plugue-a no USB e solte. Uma unidade **RP2350** aparece.
2. Copie o arquivo ``$relName`` para essa unidade. A Pico reinicia sozinha.
3. Pareie o controle: remova-o do Bluetooth do Windows, segure o BOOTSEL por 2 s (o LED pisca rapido) e segure o botao Sync do controle.

Depois, basta apertar um botao do controle para reconectar. Documentacao completa no [README](../README.md).

## Destaques
- Todos os botoes, sticks, giroscopio/acelerometro, vibracao HD e LEDs de jogador.
- Reconexao automatica, pareamento pelo BOOTSEL, wake do PC da suspensao e luz do Home como indicador (conectado/bateria).
- Latencia baixa: o dongle acrescenta menos de 0,5 ms (poll USB de 1 ms, fila sem perda de relatorios).

## Limitacoes
Um controle por dongle. Testado no Windows 11 com um Pro Controller original; Linux, macOS e clones nao foram testados.

---

# English

USB receiver for the **Nintendo Switch Pro Controller** using a **Raspberry Pi Pico 2 W**. The Pico talks to the controller
over Bluetooth and shows up on the PC as a native USB Pro Controller (057E:2009).

> **Requires a Raspberry Pi Pico 2 W** (RP2350 + Bluetooth). The plain Pico 2 (no "W") has no Bluetooth and will not work.

**Install:** hold BOOTSEL while plugging the Pico in, copy ``$relName`` to the **RP2350** drive. To pair: remove the controller
from Windows Bluetooth, hold BOOTSEL for 2 s (fast LED blink), then hold the controller's Sync button.
``$dbgName`` adds a serial log port. Verify downloads with ``SHA256SUMS.txt``.
"@
$notes | Set-Content "$dist\RELEASE_NOTES-$tag.md" -Encoding utf8

Write-Host ""
Write-Host "Release $tag pronto em $dist :"
Get-ChildItem $dist | ForEach-Object { "  {0,-42} {1,9:N0} bytes" -f $_.Name, $_.Length }
Write-Host ""
Get-Content "$dist\SHA256SUMS.txt"
