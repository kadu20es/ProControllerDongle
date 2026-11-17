# Fase 2 — resultados (2026-11-16)

Pareamento e HID host funcionando entre o Pico 2W e um Pro Controller original.

## Fluxo que funciona
1. Inquiry (~30 s, repete sozinho) com o controle em modo Sync; filtro por CoD `0x002508` (gamepad).
2. `gap_dedicated_bonding` (SSP, Just Works) → chave de link gravada na flash (TLV do SDK).
3. O bonding derruba o ACL (motivo 0x16). Esperar 1 s e chamar `hid_host_connect` → conecta.
4. Pre-requisito: o controle NAO pode estar pareado/conectado ao Windows (ele reconecta ao PC).

## Dados observados
- Controle de teste e dongle: enderecos Bluetooth omitidos.
- Descritor HID via SDP: 170 bytes (o log da fase 2 truncou em 64; limite subido para 256).
  Report IDs vistos no inicio: 0x21, 0x30, 0x31, 0x32, 0x33, 0x3F.
- `HID_SUBEVENT_REPORT` entrega o relatorio **com o byte de cabecalho `0xA1` na frente**
  (ex.: `a1 3f ...`, `a1 21 ...`): o report ID fica no indice 1.
- Modo padrao apos conectar: relatorios `0x3F` (simples, 13 bytes com o header), ~67/s (inclui ruido dos sticks).
- Subcomando `0x02` OK: resposta `0x21`, firmware `03 49`, tipo `03` (Pro Controller), MAC no payload.
- Subcomando `0x30` (LED jogador 1) OK: o controle acendeu o LED 1 → escrita BT funciona.
- `hid_host_send_report(cid, 0x01, buf[48])` e o formato certo para subcomandos.

## Reconexao sem Sync (resolvida, 2026-11-16)
- Antes: apertar um botao nao reconectava. Depois de: (1) parar o inquiry quando ha bond, (2) ficar connectable e
  aguardar, (3) retry ativo a cada 4 s, o controle reconecta: log `HCI connection request` -> `Conexao HID de entrada`
  -> relatorios a ~67/s e subcomandos OK. O bond sobrevive a regravacao do firmware.
- Causa exata nao isolada: o teste anterior tambem tinha o controle ainda associado ao Windows e o dongle ficava em inquiry.
- Comandos serial: `p` esquece o bond e pareia de novo; `s` mostra o estado.
- Descritor HID completo (170 bytes) em `docs/pro-controller-bt-hid-descriptor.txt`.

## Proximo (fase 3)
Modo completo `0x30`: ler SPI (`0x10`) para calibracao/cores, `0x03`→`0x30`, IMU `0x40`, vibracao `0x48`.
