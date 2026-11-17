# Changelog

Formato baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/). Versionamento [SemVer](https://semver.org/lang/pt-BR/).

## [1.0.0] — 2026-11-17

Primeira versão pública. Requer **Raspberry Pi Pico 2 W**.

### Adicionado
- Ponte Bluetooth Classic (controle) → USB (PC): o dongle aparece como Nintendo Switch Pro Controller (`057E:2009`).
- Pareamento e reconexão automática; pareamento sem o PC pelo botão BOOTSEL (2 s); pareamento salvo na flash.
- Protocolo completo do controle: leitura da SPI (calibração), IMU, vibração HD, LEDs de jogador e modo de relatório `0x30`.
- Handshake USB `0x80`, emulação dos subcomandos e descritores copiados do controle original.
- Acordar o PC da suspensão (USB remote wakeup, com teclado HID de apoio que envia F15).
- Luz do botão Home como indicador: fraca e fixa quando conectado; pisca devagar com bateria crítica.
- LED de estado na placa (conectado, pareando, aguardando).
- Relatório "solto" ao desconectar (sem botões presos no PC).
- Latência baixa: poll USB de 1 ms e fila de 8 relatórios entre o Bluetooth e o USB.
- Build **release** (sem serial) e **debug** (serial CDC com log e comandos), scripts de build, gravação e medição.
- Documentação em `docs/` e README em português e inglês.

### Conhecido
- Um controle por dongle. Testado só no Windows 11 com um Pro Controller original.
- O "Sinalizar" do Steam não gera vibração (o Steam não envia o comando); a vibração em jogos funciona.
