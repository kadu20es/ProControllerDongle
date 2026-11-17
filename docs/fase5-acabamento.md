# Fase 5 — acabamento (2026-11-16)

## O que foi feito
- **Builds**: `tools\build.cmd` (debug, com CDC de log, pasta `firmware\build`) e `tools\build.cmd release`
  (`-DPROCON_RELEASE=ON`: sem CDC, log mudo, descritor USB identico ao Pro Controller original, pasta `firmware\build-release`).
  `tools\flash.ps1 [-Release]` grava; no debug reinicia em BOOTSEL abrindo a serial a 1200 baud.
  **O release nao tem serial**: para regravar, BOOTSEL manual (segure ao plugar) ou voltar ao debug.
  Se a serial de log estiver aberta por outro programa (ex.: `serial-log.ps1`), o reboot a 1200 baud falha em silencio.
- **LED de estado** (LED da placa, tick de 100 ms): aceso fixo = controle conectado; piscando rapido (5 Hz) = pareando/buscando;
  pulso curto a cada 2 s = pareado, aguardando o controle (aperte um botao).
- **Pareamento por botao**: BOOTSEL 2 s apaga o pareamento e abre a busca (sem PC). Comando serial `p` faz o mesmo.
  Bug corrigido: com o controle conectado a busca nao iniciava porque o HID ainda nao tinha fechado; agora a busca comeca em
  `HID_SUBEVENT_CONNECTION_CLOSED` (se nao ha bond). Teste real: BOOTSEL 2 s + Sync -> pareou e reconectou (usuario confirmou).
- **Relatorio neutro ao desconectar**: ao perder o controle, o dongle envia um 0x30 com botoes em zero e sticks no centro calibrado,
  para nao deixar botao preso no PC (implementado em `procon.c`; nao verificado em teste dedicado).
- Leitura do BOOTSEL via QSPI CS (funcao em RAM com interrupcoes desligadas por ~10 us).

## Decisoes
- **Um controle por dongle** (decisao do usuario; so ha 1 controle). Dois controles = dois dongles ou fase 7 opcional
  (varios slots + uma interface HID USB por slot; precisa validar Steam/SDL com 2 interfaces de mesmo VID/PID).
- Com o controle desligado o PC continua vendo o dispositivo USB (sem entrada), como um controle com fio sem uso.
- Linux: deve funcionar com o driver `hid-nintendo` (nao testado).

## Pendencias
- Steam: "Sinalizar" nao gerou vibracao (testar num jogo).
- IMU em jogo/emulador com mira por movimento; caixa de movimento do Eden "torta" na mao (provavelmente a pegada, nao conclusivo).
- Medir latencia BT->USB; avaliar 120 Hz de USB (hoje ~67/s, limite do Bluetooth).
- Repositorio git proprio para o projeto (o git detectado e o de `E:\Dev`).

## Acordar o PC da suspensao (2026-11-17)
- **Como**: `usb_procon.c` usa USB remote wakeup (`tud_remote_wakeup`) quando o USB esta suspenso e o controle (a) conecta por BT ou
  (b) manda um relatorio com algum botao apertado. Depois de retomar, envia F15 por um teclado HID virtual para acender a tela.
- **Por que um teclado**: o Windows so oferece/ativa "permitir que este dispositivo ative o computador" para HID com cara de teclado/mouse
  (mesma ideia do DS5Dongle). A interface de teclado e a de indice 1 (`ITF_KBD`); o Pro Controller continua na interface 0, o CDC de log vem depois.
  O descritor deixa de ser identico ao original (dispositivo composto). Desligar: `-DPROCON_WAKE=OFF`.
- **Windows**: nao precisou configurar nada (`MSPower_DeviceWakeEnable` ja vinha `True` para a interface do teclado).
- **Teste real**: PC suspenso (S3), controle desligado, botao A -> o PC acordou e a tela acendeu ("ficou perfeito", usuario).
- No build debug, o log nao sai durante a suspensao (CDC desconectado); so depois de acordar.

## Luz do botao Home (subcomando 0x38)
- Antes o dongle so dava ACK ao 0x38 do PC e nao repassava ao controle; a luz nunca acendia. Agora `procon_forward_home_light` repassa
  ate 37 bytes de argumento. Teste com `tools\home-light.ps1 -State on|off|pulse`: acendeu/pulsou/apagou (usuario confirmou).
- Formato: byte0 = (mini ciclos << 4) | duracao global; byte1 = (intensidade inicial << 4) | ciclos (0 = infinito).
  Fixo 100% = `01 F0 F0`; apagado = `01 00 00`. O Steam usa exatamente esses.
