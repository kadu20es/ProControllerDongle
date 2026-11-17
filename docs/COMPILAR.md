# Compilar, gravar e depurar

O firmware é um projeto CMake para o Pico SDK. As versões abaixo foram as usadas no desenvolvimento (Windows 11).

## Ferramentas

| Ferramenta | Versão testada | Observação |
|---|---|---|
| Pico SDK | **2.3.1** (com submódulos: TinyUSB, BTstack, cyw43-driver, lwIP, mbedtls) | `git clone --branch 2.3.1 --recurse-submodules https://github.com/raspberrypi/pico-sdk` |
| GCC ARM (`arm-none-eabi-gcc`) | 15.2.1 (xPack) | Arm GNU Toolchain 13+ também deve servir |
| CMake | ≥ 3.13 | |
| Ninja | 1.13 | |
| Compilador C/C++ do PC | MSVC Build Tools | Só para compilar o `pioasm` do SDK. Qualquer compilador do PC serve |
| picotool | 2.3.1 | Opcional (o SDK usa para empacotar) |

A placa-alvo é `pico2_w` e a plataforma `rp2350-arm-s` (já definidas no `firmware/CMakeLists.txt`).

## Compilar (Windows)

1. Copie `tools\env.example.cmd` para `tools\env.cmd` (ignorado pelo git) e ajuste os caminhos
   (`PICO_SDK_PATH` é obrigatório; os demais são opcionais se as ferramentas já estiverem no `PATH`).
2. Rode:

```bat
tools\build.cmd release        rem final  -> firmware\build-release\procontroller_dongle.uf2
tools\build.cmd                rem debug  -> firmware\build\procontroller_dongle.uf2
tools\build.cmd release clean  rem limpa antes de compilar
```

O script localiza o compilador do PC pelo `vswhere` se o MSVC não estiver no `PATH`.

### Compilar manualmente (qualquer sistema)

```sh
cd firmware
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DPICO_SDK_PATH=/caminho/pico-sdk -DPROCON_RELEASE=ON
cmake --build build-release
```

Os scripts de gravação e de teste em `tools/` são PowerShell (Windows). Em outros sistemas, grave copiando o `.uf2`
para a unidade `RP2350` (BOOTSEL).

## Gravar

- **Primeira vez / qualquer build:** segure **BOOTSEL**, plugue a Pico, copie o `.uf2` para a unidade `RP2350`.
- **Com o build debug já gravado:** `powershell -File tools\flash.ps1` reinicia a Pico em BOOTSEL abrindo a porta
  serial a 1200 baud e copia o `.uf2` sozinho. Use `-Release` para o firmware final.
- O **release não tem porta serial**, então regravar a partir dele exige o BOOTSEL manual.
- Se `flash.ps1` falhar em silêncio, outro programa (como o `serial-log.ps1`) pode estar com a porta serial aberta.

## Build debug: log e testes

O build debug adiciona uma **porta serial** (CDC) ao dispositivo USB, com log e comandos
([CONFIGURACAO.md](CONFIGURACAO.md)).

| Script (`tools/`) | Para que serve |
|---|---|
| `serial-log.ps1 [-Seconds N]` | Lê o log do dongle (reconecta se a porta sumir) |
| `flash.ps1 [-Release]` | Grava o firmware (veja acima) |
| `imu-test.ps1` | Lê relatórios `0x30` direto do HID e mostra a atividade do IMU por segundo |
| `imu-phases.ps1` | Mede o IMU em 3 posições (com beeps) |
| `latency-host.ps1` | Cadência, jitter e perdas de relatórios vistas pelo PC |
| `home-light.ps1` | Acende/apaga o anel do Home (só tem efeito com `PROCON_HOME_PASSTHROUGH=ON`) |

O log do firmware debug inclui a versão no início (`=== Pro Controller Dongle vX.Y.Z ===`), os eventos de
Bluetooth/HID, os pedidos do PC e, a cada 5 s, estatísticas de latência (`LAT 5s:`).

## Estrutura do código

```
firmware/
  CMakeLists.txt        projeto, opções de compilação e versão
  src/main.c            Bluetooth (pareamento, reconexão), LED de estado, botão BOOTSEL, comandos seriais
  src/procon.c/.h       protocolo BT do controle: setup, imagem da SPI, calibração, IMU, vibração, luz do Home
  src/usb_procon.c/.h   TinyUSB: descritores, handshake 0x80, subcomandos, fila BT->USB, wake, CDC de log
  src/btstack_config.h  configuração do BTstack
  src/tusb_config.h     configuração do TinyUSB
tools/                  scripts de build, gravação e teste
docs/                   documentação
```

Detalhes de funcionamento em [ARQUITETURA.md](ARQUITETURA.md).
