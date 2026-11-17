# ProControllerDongle — Plano e estado

Pico 2W faz ponte: Nintendo Switch Pro Controller (Bluetooth Classic) -> USB nativo (057E:2009) no PC.

## Estado (2026-11-16)
| Fase | Resultado |
|---|---|
| 1. Ambiente (toolchain, build, gravacao) | concluida — `tools\build.cmd`, `tools\flash.ps1` |
| 2. Bluetooth (pareamento, reconexao, HID host) | concluida — `docs/fase2-resultados.md` |
| 3. Protocolo (SPI, IMU, vibracao, modo 0x30) | concluida — `docs/fase3-resultados.md` |
| 4. USB nativo (descritores, handshake, repasse) | concluida — `docs/fase4-resultados.md` |
| 5. Acabamento (LED, BOOTSEL, release) | concluida — `docs/fase5-acabamento.md` |
| 6. Opcional: 2+ controles por dongle | nao iniciada (decisao: 1 controle por dongle) |

Validado: Windows (Chrome Gamepad API no navegador externo com o build release: botoes, sticks, vibracao), Steam (reconhece, handshake),
Eden/SDL (botoes, IMU ligado), IMU lido no PC. Nao testado: Linux (`hid-nintendo`), vibracao dentro do Steam, IMU em jogo.

## Como usar
- Pareamento: remova o controle do Bluetooth do Windows; segure BOOTSEL 2 s (LED pisca rapido) e segure Sync no controle.
- Reconexao: aperte um botao do controle (LED do dongle fica aceso fixo).
- Build debug: `tools\build.cmd` (+ `tools\flash.ps1`) — porta serial de log (`tools\serial-log.ps1`).
- Build final: `tools\build.cmd release` + `tools\flash.ps1 -Release` — sem serial; regravar exige BOOTSEL manual.

## Arquitetura
```
Pro Controller --BT Classic HID--> main.c (BTstack host) --> procon.c (setup, SPI, calibracao)
                                                          --> usb_procon.c (TinyUSB: HID 057E:2009 + CDC debug) --> PC
PC --rumble/LED--> usb_procon.c --> procon.c --> controle
```

## Riscos / pendencias
- Steam nao vibrou no teste "Sinalizar" (o Steam nao enviou pacote); testar num jogo.
- Latencia BT->USB nao medida; BT entrega ~67 relatorios/s.
- Linux nao testado.

## Referencias
- DS5Dongle: https://github.com/awalol/DS5Dongle
- dekuNukem Nintendo_Switch_Reverse_Engineering (bluetooth_hid_notes, subcommands_notes, spi_flash_notes, USB-HID-Notes)
- Dump do descritor USB do Pro Controller: gist tmk/9725596c6726b561ad2bf1f1e07e1523
