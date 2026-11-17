# Latencia — medicoes (2026-11-17)

Ferramentas: carimbos no firmware debug (`LAT 5s:` no log serial, `usb_procon.c`) e `tools\latency-host.ps1` (PC).

## Resultados (controle parado, ~30 s, Steam aberto em segundo plano)
Dongle (cada janela de 5 s, ~333 relatorios):
| Metrica | Valor |
|---|---|
| Fila do firmware (chegada BT -> `tud_hid_report`) | p50 < 0,25 ms, p95 < 0,5 ms (picos raros de 5-7 ms) |
| Ate o host ler (inclui a espera do poll USB de 8 ms) | p50 < 4,5 ms, p95 < 8,5 ms |
| Intervalo entre relatorios BT | p50 15 ms, p95 17 ms; 5-16 de ~333 intervalos >= 25 ms; max 31-46 ms |
| Relatorios descartados (sobrescritos em rajada) | 0-2 por 5 s (< 0,6%) |

PC (`latency-host.ps1`, 3 execucoes de 30 s): 66,8-66,9 relatorios/s; intervalo medio 14,96-14,98 ms, mediana 16, p95 16;
p99 24-32 ms; max 40-48 ms; intervalos >= 25 ms: 19-49 (1-2,4%); perdidos pelo contador do controle: 0-2 (<= 0,1%).
O jitter do PC inclui o escalonamento do Windows/PowerShell (limite superior).

## Leitura
- O dongle em si acrescenta < 0,5 ms (p95). A espera do poll USB (bInterval 8 ms, igual ao controle original) soma em media ~4 ms (max 8 ms).
- O piso do sistema e o Bluetooth: 1 relatorio a cada 15 ms (~67/s) e jitter ocasional (>= 25 ms em 1-5% dos intervalos), fora do controle do firmware.
- Estimativa (sem o tempo interno do controle e do jogo/monitor): media ~12 ms (7,5 de espera BT + ~0,5 dongle + ~4 poll USB); pico tipico ~25-30 ms.
- Primeira janela, logo apos conectar (setup/Steam sondando): 7,6% descartados e gap BT p95 44 ms — pior que o regime normal.

## Depois das melhorias (poll USB de 1 ms + fila de 8 relatorios), mesmo teste
| Metrica | Antes | Depois |
|---|---|---|
| Ate o host ler: mediana / p95 / max | < 4,5 / < 8,5 / < 12 ms | **< 1,0-1,25 / < 1,0-1,5 / < 2-3,25 ms** |
| Fila do firmware: max | 5-7 ms | 1,75-2,25 ms |
| Relatorios descartados (por 5 s) | 0-2 | **0** em todas as janelas |
| PC: relatorios/s, intervalo mediana / p95, perdidos | 66,8-66,9; 16 / 16 ms; 0-2 | 66,8; **15 / 16 ms; 0** |
O jitter do Bluetooth (gaps >= 25 ms em 1-4% dos intervalos, max ~45 ms) e o mesmo de antes: e do enlace, nao do dongle.
Estimativa nova (sem o controle e o jogo): media ~9 ms (7,5 de espera BT + ~0,5 dongle + ~1 USB).
`HID_POLL_MS` (padrao 1) controla o intervalo do endpoint; `-DHID_POLL_MS=8` volta ao descritor identico ao original.

## Melhorias possiveis (aplicadas: 1 e 2)
1. `bInterval` do endpoint IN de 8 ms para 1 ms: tira ~3,5 ms da media (deixa de ser identico ao descritor original).
2. FIFO curta (8) em vez de 1 slot: nao perder toque curto de botao quando chegam relatorios em rajada apos um gap BT.
3. Dongle longe de portas USB 3.0/hubs e de Wi-Fi 2,4 GHz (jitter BT).
