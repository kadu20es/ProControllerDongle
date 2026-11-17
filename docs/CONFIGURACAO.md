# Opções de compilação e comandos de depuração

## Opções de compilação

Passe ao CMake com `-DNOME=valor` (ou edite o `build.cmd`). Os valores padrão são os dos firmwares publicados.

| Opção | Padrão | Efeito |
|---|---|---|
| `PROCON_RELEASE` | `OFF` (o `tools\build.cmd release` liga) | `ON`: sem porta serial de log e sem mensagens; é o firmware de uso diário. `OFF`: build debug, com serial (CDC), log e comandos |
| `PROCON_WAKE` | `ON` | Inclui o teclado HID (só envia F15) que permite acordar o PC da suspensão. `OFF`: dispositivo sem teclado |
| `PROCON_HID_POLL_MS` | `1` | Intervalo de poll do endpoint HID, em ms. `1` dá a menor latência; `8` é o valor do controle original |
| `PROCON_HOME_PASSTHROUGH` | `OFF` | `ON`: repassa ao controle os comandos de luz do Home enviados pelo PC. `OFF`: o dongle controla a luz sozinho (fixa/fraca e pisca com bateria baixa) |

Exemplo, release com o descritor mais próximo do original (sem teclado de wake e com poll de 8 ms):

```sh
cmake -S firmware -B firmware/build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DPICO_SDK_PATH=... \
      -DPROCON_RELEASE=ON -DPROCON_WAKE=OFF -DPROCON_HID_POLL_MS=8
```

A versão do firmware fica em `PROCON_VERSION` no `firmware/CMakeLists.txt`; ela é gravada no `.uf2`
(`picotool info -a arquivo.uf2`) e impressa no log do build debug.

## Comandos da porta serial (somente build debug)

Abra a porta serial do dongle (115200 baud, qualquer valor serve) e digite uma letra:

| Tecla | Ação |
|---|---|
| `p` | Esquece o pareamento e abre a busca (igual ao BOOTSEL por 2 s) |
| `s` | Mostra o estado (conectado, conectando, buscando, pareado) |
| `v` | Liga/desliga o log detalhado (amostras do `0x30`, taxa, comandos do PC) |
| `r` | Pulso curto de vibração no controle (250 ms) |
| `d` | Mostra a imagem da SPI do controle (calibração, cores, informações) |
| `b` | Simula bateria baixa (testa o pisca da luz do Home); digite de novo para desfazer |

Abrir a porta serial a **1200 baud** reinicia a Pico em modo BOOTSEL (é o que `tools\flash.ps1` usa).

## Linhas do log

| Prefixo | Significado |
|---|---|
| `Bluetooth pronto`, `Bond encontrado`, `Inquiry:` | Início e busca |
| `HID conectado`, `Setup completo`, `Info:` | Conexão e configuração do controle |
| `BTN ...` | Mudança de botões (com nomes) |
| `0x30 t=...` | Amostra a cada 500 ms: botões, sticks (%), acelerômetro e giroscópio |
| `rate:` | Relatórios por segundo (total e `0x30`) |
| `USB: ...`, `USB OUT ...` | Handshake e pedidos do PC |
| `USB rumble:`, `FWD rumble -> BT` | Vibração do PC e seu repasse ao controle |
| `LAT 5s:` | Estatísticas de latência do dongle (a cada 5 s) |
| `Luz do Home:` | Estado do indicador do anel do Home |
