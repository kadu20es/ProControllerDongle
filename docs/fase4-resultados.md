# Fase 4 — resultados (2026-11-16)

O dongle aparece no PC como **Nintendo Switch Pro Controller (057E:2009)** e repassa entrada, vibracao e LED.

## Arquitetura (firmware/src)
- `main.c`: Bluetooth (pareamento, reconexao, HID host).
- `procon.c`: protocolo BT do controle (setup, imagem SPI 0x6000-0x60FF/0x8000-0x803F, calibracao, repasse PC->controle).
- `usb_procon.c`: descritores USB identicos ao original (203 B de report descriptor, bcdDevice 2.10, serial 000000000001),
  handshake 0x80 (0x01/0x02/0x03 respondem 0x81; 0x04/0x05 sem resposta), subcomandos emulados (0x02, 0x03, 0x04, 0x10 via cache,
  0x30 -> LED real; os demais so ACK), CDC de log (stdio) e reboot por serial a 1200 baud.
- `tusb_config.h`: `PROCON_DEBUG_CDC=1` adiciona a porta serial (dispositivo composto). Com 0 o descritor fica igual ao original.

## Validado
- Windows reconhece (HID game controller + COM de log). `joy.cpl` mostra botoes/eixos "loucos": esperado, o descritor generico
  nao descreve o relatorio 0x30 (timer/bateria viram botoes 1-8). Steam/SDL/Chrome leem o formato bruto.
- **Steam**: reconhece como "Nintendo Switch Pro Controller" e completa o handshake. Manda `0x80 0x04` ~5x/s (keep-alive) e quase nada alem;
  *Identificar controle > Sinalizar* e o controle de intensidade NAO geraram pacote de vibracao (limitacao do Steam, nao do dongle).
- **Eden (SDL)**: handshake, SPI, 0x48, 0x38, 0x30 e liga/desliga do IMU (0x40); botao L reconhecido.
- **Chrome (Gamepad API, hardwaretester.com)**: "STANDARD GAMEPAD", vibracao suportada; teste de 1 s **vibrou** (usuario confirmou).
  O log mostra `USB rumble` -> `FWD rumble -> BT status 0x00`.
- Vibracao direta (script PowerShell escrevendo no HID): vibrou (usuario confirmou).

## Descobertas
- O controle real em USB envia 0x30 por padrao; o dongle liga o fluxo ao receber `0x80 0x04` (ou `0x03 0x30`).
  Sem isso o host fica reenviando `0x80 0x04` em loop.
- Reboot a 1200 baud: o reset direto dentro do callback travava o Pico; agora `tud_disconnect()` + 100 ms + `reset_usb_boot` no loop principal.
  Funcionou nas 3 ultimas gravacoes (`tools/flash.ps1`). Se falhar: BOOTSEL manual.
- Windows: o PnP do dongle e `USB\VID_057E&PID_2009` (COM de log em MI_01); `tools/serial-log.ps1` procura por esse ID.

## Pendencias
- Steam nao vibra no teste "Sinalizar": verificar num jogo real.
- ~~Giroscopio/IMU~~ **Validado** com `tools/imu-test.ps1` (leitura direta do HID no PC, 16 s): parado, acc Z = 4111..4135
  (~1 g, 4096/g) e gyro com offset fixo (~60, 78, 6); girando, o gyro chega a milhares (ate ~19500) e acc Z varia de -19534 a 32362.
  Teste de 3 posicoes (`tools/imu-phases.ps1`): mesa e bandeja dao inclinacao de 2-6 graus com a origem de fabrica (acc Z ~4134 = 1 g);
  a fase "duas maos reta" saiu identica a mesa (controle nao saiu do lugar), entao nao mediu a pegada. Usuario relatou no Eden a caixa
  de movimento reta na mesa e torta na mao: atribuido a pegada/inclinacao natural, calibracao parece correta (nao conclusivo).
  Fluxo USB estavel em 63-67 relatorios/s (3 amostras de IMU cada, ~200 amostras/s). Falta so testar num jogo/emulador com mira por movimento.
- Sem controle BT conectado o PC ainda ve o dispositivo (sem entrada). Avaliar comportamento ideal.
- Reduzir ruido de log (verbose off por padrao), build "release" sem CDC, LED de status, botao BOOTSEL (pareamento), 2 controles (opcional).
