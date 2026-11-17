# Protocolo do Pro Controller usado pelo dongle

Resumo do que o firmware usa, validado com um Pro Controller original (firmware 3.49). A referência da comunidade é o
repositório [Nintendo_Switch_Reverse_Engineering](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering).

## Bluetooth (dongle ⇄ controle)

- Bluetooth Classic, **HID** (L2CAP PSM `0x11` controle e `0x13` interrupção). Classe de dispositivo do controle
  `0x002508` (periférico / gamepad). Pareamento SSP "Just Works". Modo de protocolo: *report*.
- O BTstack entrega cada relatório de entrada **com o byte `0xA1`** (cabeçalho DATA/INPUT) na frente: `r[0]=0xA1`,
  `r[1]=ID do relatório`.
- Conexão logo após parear: o controle manda o relatório simples `0x3F`. O dongle o coloca em modo completo.

### Relatórios de entrada
| ID | Uso |
|---|---|
| `0x3F` | Simples (padrão no Bluetooth). Ignorado |
| `0x21` | Resposta a subcomando |
| `0x30` | **Completo**: botões, sticks, IMU. ~67 por segundo (um a cada ~15 ms) |

### Relatório `0x30` (índices contando o `0xA1` em `r[0]`; no USB subtraia 1)
| Bytes | Conteúdo |
|---|---|
| `r[2]` | Contador de tempo (tick de ~5 ms; ~3 por relatório) |
| `r[3]` | Bateria e conexão: nibble alto `8` cheio, `6` médio, `4` baixo, `2` crítico, `0` vazio; bit 4 = carregando |
| `r[4..6]` | Botões: direita, compartilhado, esquerda |
| `r[7..9]` | Stick esquerdo (2 × 12 bits) |
| `r[10..12]` | Stick direito (2 × 12 bits) |
| `r[13]` | Vibrador |
| `r[14..49]` | IMU: 3 amostras × (acc X Y Z, gyro X Y Z), `int16` little-endian |

Botões (bits): **direita** `Y=01 X=02 B=04 A=08 SR=10 SL=20 R=40 ZR=80` · **compartilhado**
`Minus=01 Plus=02 R3=04 L3=08 Home=10 Capture=20` · **esquerda** `Baixo=01 Cima=02 Direita=04 Esquerda=08 SR=10 SL=20 L=40 ZL=80`.

Stick: `x = b0 | (b1 & 0x0F) << 8`, `y = (b1 >> 4) | b2 << 4`. A calibração vem da SPI.

### Relatórios de saída
| ID | Formato | Uso |
|---|---|---|
| `0x01` | contador (4 bits) + 8 bytes de vibração + subcomando + argumentos (48 bytes) | Subcomandos |
| `0x10` | contador + 8 bytes de vibração (9 bytes) | Só vibração |

Vibração (4 bytes por lado, esquerdo depois direito). Neutro: `00 01 40 40`. Exemplo moderado (160 Hz, ~0,55):
`80 90 40 64`. **Evite amplitudes altas** (acima de ~1,0): podem danificar os atuadores.

### Subcomandos usados
| Sub | Argumentos | Uso |
|---|---|---|
| `0x02` | — | Informações: firmware, tipo (3 = Pro Controller), MAC |
| `0x10` | endereço (4 B LE) + tamanho (≤ 0x1D) | Leitura da SPI. Resposta com ACK `0x90` |
| `0x40` | `01` | Liga o IMU |
| `0x48` | `01` | Liga a vibração |
| `0x03` | `30` | Modo de relatório completo |
| `0x30` | máscara | LED de jogador (nibble alto aceso, baixo piscando) |
| `0x38` | até 37 B | **Luz do botão Home**. Fixa: `01 F0 F0` (100%); fraca ~20%: `01 30 30`; pisca lento do dongle: `2F 00 30 28 28` |

Luz do Home: byte0 = (mini-ciclos << 4) | duração global (≈175 ms no máximo); byte1 = (intensidade inicial << 4) |
nº de ciclos (0 = infinito); depois pares de bytes por mini-ciclo (intensidade, e multiplicadores de fade/duração).

### SPI (somente leitura)
| Endereço | Conteúdo |
|---|---|
| `0x6000` (16 B) | Número de série |
| `0x6020` (24 B) | Calibração do IMU de fábrica |
| `0x603D` (18 B) | Calibração dos sticks de fábrica (esquerdo 9 B, direito 9 B) |
| `0x6050` (12 B) | Cores |
| `0x6080` (24 B) | Offsets do IMU e parâmetros dos sticks |
| `0x8010`… | Calibração de usuário dos sticks (magic `B2 A1` quando existe) |
| `0x8026`… | Calibração de usuário do IMU |

Calibração do stick (9 bytes → seis valores de 12 bits): esquerdo = `[xmax-Δ, ymax-Δ, xcentro, ycentro, xmin-Δ, ymin-Δ]`;
direito = `[xcentro, ycentro, xmin-Δ, ymin-Δ, xmax-Δ, ymax-Δ]`.

## USB (dongle ⇄ PC)

- **Dispositivo:** `057E:2009`, `bcdDevice 2.10`, "Nintendo Co., Ltd." / "Pro Controller", serial `000000000001`,
  alimentado pelo barramento (500 mA, remote wakeup).
- **Interface 0:** HID, 2 endpoints interrupção de 64 bytes (IN `0x81`, OUT `0x01`), report descriptor de 203 bytes
  copiado do controle original. Interface 1 (opcional): teclado HID. Depois, o CDC de log (debug).
- **IDs de relatório:** entrada `0x30` (estado), `0x21` (resposta a subcomando), `0x81` (resposta `0x80`);
  saída `0x01` (subcomando), `0x10` (vibração), `0x80` (comando USB), `0x82`.

### Handshake `0x80` (relatório de saída → resposta `0x81`)
| Comando | Resposta |
|---|---|
| `80 01` | `81 01 00 03 <MAC 6 B>` (tipo 3 = Pro Controller) |
| `80 02` | `81 02` (handshake) |
| `80 03` | `81 03` (baud rate) |
| `80 04` | nenhuma — força USB sem timeout; o controle passa a enviar `0x30` por padrão |
| `80 05` | nenhuma — volta a permitir timeout |

Depois do handshake o PC envia subcomandos `0x01`; o dongle responde com `0x21`:
`[0x21][timer][bat][botões 3][sticks 6][vib][ACK][sub][dados…]`.

O descritor HID **não descreve** o relatório `0x30` real (só os IDs `0x30`, `0x21`, `0x81`, `0x01`, `0x10`, `0x80`,
`0x82`); programas que entendem o Pro Controller leem o formato bruto. É o mesmo comportamento do controle original.
