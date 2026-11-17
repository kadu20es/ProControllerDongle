# Fase 3 — resultados (2026-11-16)

Protocolo completo do Pro Controller sobre Bluetooth validado (`firmware/src/procon.c`).

## Sequencia pos-conexao (funciona, cada passo espera a resposta 0x21)
1. `0x02` info (firmware 3.49, tipo 3 = Pro Controller)
2. `0x10` SPI: 0x6000 serial, 0x6020 cal IMU, 0x603D cal sticks (18 B), 0x6050 cores, 0x6080 sensor/stick params,
   0x8010 cal usuario sticks, 0x8026 cal usuario IMU
3. `0x40`=1 IMU, `0x48`=1 vibracao, `0x03`=0x30 modo completo, `0x30`=0x01 LED jogador 1
- Espera de 1,5 s apos o HID abrir antes do primeiro comando.

## Medicoes
- Relatorio `0x30`: **~67/s** via Bluetooth (66-68, estavel; nao chega aos 120 Hz da doc). Cada relatorio traz 3 amostras de IMU.
- Layout (com o byte `0xA1` na frente): [1]=0x30 [2]=timer [3]=bat/conn [4..6]=botoes (direita, compartilhado, esquerda)
  [7..9]=stick esq. [10..12]=stick dir. [13]=vibrador [14..49]=IMU 3x(acc xyz, gyr xyz) int16 LE.
- Stick: 12 bits; calibracao de fabrica decodificada (esq.: c(2139,2019); dir.: c(2025,2191) neste controle) → ±100% OK.
- IMU em repouso: acc Z ≈ 4100 (≈1 g), gyr ≈ 0; responde a movimento.
- Controle de teste: serial SPI vazio (ff), cores corpo #323232 / botoes #ffffff, sem calibracao de usuario.
- Vibracao: `0x10` com 80 90 40 64 por lado (160 Hz, amp ~0.55), 250 ms → **usuario confirmou que vibrou**.
- Botoes (grupo r[4] r[5] r[6]): vistos Esquerda=000008, X=020000, Y=010000, ZR=800000, Home=001000, Direita=000004, B=040000
  — coerente com a documentacao. **Teste completo com log por mudanca (BTN): os 18 botoes foram reconhecidos**
  (Cima, Baixo, Esquerda, Direita, A, B, X, Y, L, R, ZL, ZR, L3, R3, Minus, Plus, Home, Capture) (direita: Y1 X2 B4 A8 SR10 SL20 R40 ZR80; comum: Minus1 Plus2 R3 4 L3 8 Home10 Capture20;
  esquerda: Baixo1 Cima2 Direita4 Esquerda8 SR10 SL20 L40 ZL80). A amostragem de 500 ms nao capturou todos.

## Implicacao para a fase 4
Como o dispositivo USB sera um Pro Controller nativo (mesmo layout de relatorio), **nao ha remapeamento**:
encaminhar o `0x30` BT -> USB (sem o byte `0xA1`). SPI ja esta em cache (`procon.c`) para responder as leituras do PC sem ir ao controle.
Comandos serial uteis: `r` pulso de vibracao, `d` dump SPI, `s` estado, `p` esquecer bond.
