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

## Melhorias possiveis
1. `bInterval` do endpoint IN de 8 ms para 1 ms: tira ~3,5 ms da media (deixa de ser identico ao descritor original).
2. FIFO curta (8) em vez de 1 slot: nao perder toque curto de botao quando chegam relatorios em rajada apos um gap BT.
3. Dongle longe de portas USB 3.0/hubs e de Wi-Fi 2,4 GHz (jitter BT).
