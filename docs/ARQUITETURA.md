# Arquitetura

## Visão geral

```
 Pro Controller                    Raspberry Pi Pico 2 W                          PC
┌─────────────┐   Bluetooth    ┌────────────────────────────────────┐   USB   ┌──────────┐
│ botões/IMU  │ ─────────────▶ │ main.c      BTstack HID host       │         │ Steam /  │
│ stick       │   relatórios   │   │                                │         │ emulador │
│             │      0x30      │   ▼                                │         │ / jogo   │
│ vibração    │ ◀───────────── │ procon.c   setup, SPI, calibração, │ ──────▶ │          │
│ LEDs        │   0x01 / 0x10  │   │        luz do Home, repasse    │ relat.  │ HID      │
└─────────────┘                │   ▼                                │  0x30   │ 057E:2009│
                               │ usb_procon.c  TinyUSB: HID Pro     │ ◀────── │          │
                               │   Controller + teclado de wake     │ 0x01/10 │          │
                               │   + CDC de log (debug)             │ 0x80    │          │
                               └────────────────────────────────────┘         └──────────┘
```

O dongle tem **dois papéis** ao mesmo tempo:

1. **Host Bluetooth** do controle: faz o papel do Switch (pareamento, handshake de subcomandos, modo de relatório
   completo) e recebe os relatórios.
2. **Dispositivo USB** para o PC: faz o papel de um Pro Controller ligado por cabo (descritores, handshake `0x80`,
   respostas aos subcomandos) e entrega os relatórios recebidos.

Os bytes de entrada (botões, sticks, IMU) **não são convertidos**: o relatório `0x30` do Bluetooth segue para o USB
sem o byte de cabeçalho `0xA1`. Por isso não há mapeamento de botões nem perda de recursos.

## Módulos

### `main.c` — Bluetooth e interface do dispositivo
- Inicializa `cyw43`, L2CAP, `hid_host` e o BTstack (modo Classic, SSP "just works", chaves na flash).
- **Pareamento:** inquiry filtrando a classe de dispositivo gamepad (`0x2508`), `gap_dedicated_bonding`, espera de 1 s
  (o bonding derruba o ACL) e `hid_host_connect`.
- **Reconexão:** com pareamento salvo, fica *connectable* e tenta `hid_host_connect` a cada 4 s; aceita também a
  conexão iniciada pelo controle (`HID_SUBEVENT_INCOMING_CONNECTION`).
- **Interface:** tick de 100 ms para o LED de estado, para o botão BOOTSEL (lido do QSPI CS, em RAM e com
  interrupções desligadas por ~10 µs) e para os comandos da serial.
- Quando o HID conecta chama `procon_on_connected()` e `usb_procon_request_wake()`.

### `procon.c` — protocolo do controle
- **Setup** (máquina de estados, um comando por vez, com reenvio): `0x02` (info) → leituras da **SPI** (0x6000–0x60FF e
  0x8000–0x803F, em blocos de 29 bytes) → `0x40` (IMU) → `0x48` (vibração) → `0x03` = `0x30` (modo completo) → `0x30` (LED).
- **Imagem da SPI em RAM:** serve as leituras do PC sem consultar o controle; o resto responde `0xFF`.
- **Calibração dos sticks** (fábrica ou usuário) para os logs e para o relatório "solto".
- **Luz do Home:** fixa e fraca ao conectar; pisca lento se a bateria estiver crítica (e sem carregar).
- **Repasse PC → controle:** vibração (relatório `0x10`) e LED de jogador, por *slots* protegidos por
  `critical_section` e entregues no contexto do BTstack (`btstack_run_loop_execute_on_main_thread`).
- **Relatório neutro** ao desconectar, para não deixar botões presos.

### `usb_procon.c` — lado USB
- **Descritores** do Pro Controller (device, configuração, 203 bytes de report descriptor, strings), mais o teclado
  de wake e o CDC de log conforme as opções de compilação.
- **Handshake `0x80`:** `0x01` (status, com tipo e MAC), `0x02`, `0x03` respondem com `0x81`; `0x04` liga o fluxo `0x30`.
- **Subcomandos do PC** (relatório `0x01`): `0x02` (info real do controle), `0x10` (SPI, do cache), `0x03`, `0x04`, `0x30`
  (LED, repassado), os demais recebem ACK genérico. As respostas saem como relatórios `0x21`.
- **Fila BT → USB** de 8 relatórios (ninguém é sobrescrito em rajadas do Bluetooth); o poll de 1 ms a esvazia.
- **Wake:** `tud_remote_wakeup()` quando o USB está suspenso e o controle conecta ou aperta um botão; depois envia F15.
- **Log:** `printf` vai para um *ring buffer* e é drenado pelo CDC no loop principal (driver de `stdio` próprio).
  Abrir a porta a 1200 baud reinicia em BOOTSEL.
- **Estatísticas de latência** (`LAT 5s:`) por histogramas.

## Contextos de execução

| Contexto | O que roda | Comunicação |
|---|---|---|
| Interrupção de baixa prioridade do `cyw43` ("threadsafe background") | BTstack: eventos HID, timers, `procon.c`, `usb_procon_push_input` | `critical_section` + fila `inq` |
| Laço principal (`main`) | `tud_task()` do TinyUSB, handlers do USB, drenagem do log | fila `inq` (leitura), slots de repasse (escrita) |
| IRQ do USB | TinyUSB (controle e endpoints) | — |

Regras: nada bloqueante nos handlers do BTstack; o log nunca bloqueia (descarta se o *ring* encher); o estado
compartilhado só é tocado dentro de `critical_section`.

## Variantes de build
- **release:** sem CDC e sem log; é o descritor mais enxuto.
- **debug:** adiciona a interface CDC (serial) e o log.
- Opções (`PROCON_WAKE`, `PROCON_HID_POLL_MS`, `PROCON_HOME_PASSTHROUGH`) em [CONFIGURACAO.md](CONFIGURACAO.md).

## Pareamento e flash
O BTstack guarda as chaves de link na flash (banco TLV do Pico SDK, nos últimos setores). Regravar o firmware
**preserva** o pareamento; o BOOTSEL por 2 s ou o comando `p` o apaga.
