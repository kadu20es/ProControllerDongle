# Pro Controller Dongle

[English version](README.en.md)

Um receptor **USB para o Nintendo Switch Pro Controller**, feito com uma **Raspberry Pi Pico 2 W**.
O Pico conversa com o controle por Bluetooth e aparece no PC como um **Pro Controller USB nativo**
(`057E:2009`), sem precisar do Bluetooth do computador. Funciona com Steam, emuladores e qualquer
programa que já aceite o Pro Controller por cabo.

```
Pro Controller ──Bluetooth──▶ Raspberry Pi Pico 2 W ──USB──▶ PC
 (botões, sticks, IMU)         (este firmware)          (vê um Pro Controller)
      ◀── vibração, LEDs ─────────────────◀────────────────────
```

> **Requisito de hardware: Raspberry Pi Pico 2 W.**
> O firmware precisa do chip Bluetooth (CYW43439) e do USB do **RP2350**. A **Pico 2** *sem* o "W" **não tem
> Bluetooth** e não serve. A Pico / Pico W (RP2040) não é suportada por este firmware.

Projeto independente, sem vínculo com a Nintendo, para **uso pessoal e não comercial** (veja [Créditos e licenças](#créditos-e-licenças)). "Nintendo", "Switch" e "Pro Controller" são marcas dos
seus respectivos donos. Inspirado no [DS5Dongle](https://github.com/awalol/DS5Dongle), que faz o mesmo
para o controle DualSense.

## Recursos

- Todos os botões, os dois sticks (calibração de fábrica lida do próprio controle) e o direcional.
- **Giroscópio e acelerômetro** (IMU) e **vibração HD**, repassados em ambos os sentidos.
- LEDs de jogador repassados do PC para o controle.
- Reconexão automática: aperte qualquer botão do controle.
- **Pareamento sem o PC**: segure o botão BOOTSEL da Pico por 2 s.
- **Acorda o PC da suspensão** com um botão do controle (Windows testado).
- **Luz do botão Home como indicador**: fixa e fraca quando conectado; pisca devagar com bateria crítica.
- Baixa latência: o dongle acrescenta menos de 0,5 ms; ~9 ms estimados da entrada Bluetooth até o PC
  (sem contar o controle e o jogo). Veja [docs/latencia.md](docs/latencia.md).
- Dois firmwares: **release** (uso diário, sem serial) e **debug** (porta serial de log e comandos de teste).

## O que você precisa

| Item | Detalhe |
|---|---|
| **Raspberry Pi Pico 2 W** | Obrigatória (RP2350 + Bluetooth). Cabo micro-USB **de dados** |
| Nintendo Switch Pro Controller | Testado com um original. Clones (8BitDo etc.) não foram testados |
| PC | **Windows 11** testado. Linux (`hid-nintendo`) e macOS: não testados |

## Instalação (usuário)

1. Baixe o arquivo **`ProControllerDongle-vX.Y.Z.uf2`** na página de [Releases](../../releases).
2. **Segure o botão BOOTSEL** da Pico, **plugue-a no USB** e solte. Uma unidade chamada **`RP2350`** aparece.
3. **Copie o `.uf2`** para essa unidade. A Pico reinicia sozinha. Pronto.

Para atualizar, repita os passos com o novo `.uf2`.

## Primeiro uso: parear o controle

1. **Remova o controle do Bluetooth do Windows** (se ele já foi pareado lá), senão ele volta a conectar ao PC.
2. **Segure o BOOTSEL por 2 s.** O LED da placa passa a piscar rápido (modo de pareamento).
3. **Segure o botão Sync** do controle (o pequeno, no topo) até as luzes de jogador correrem.
4. Quando o LED da Pico ficar **aceso fixo**, está conectado. O anel do botão Home fica aceso fraco.

Nas próximas vezes basta **apertar um botão** do controle. O pareamento fica salvo na memória da Pico.

## Indicadores

| LED da placa Pico | Significado |
|---|---|
| Aceso fixo | Controle conectado |
| Pisca rápido (5 Hz) | Procurando um controle em modo de pareamento |
| Pulso curto a cada 2 s | Já pareado, aguardando o controle (aperte um botão) |

| Anel do botão Home (no controle) | Significado |
|---|---|
| Aceso fraco e fixo | Conectado |
| Pisca devagar (~3,5 s por ciclo) | Bateria crítica e fora da tomada |

## Dicas de uso

- **Steam:** deixe o **Steam Input** ligado para o controle. Ele traduz o Pro Controller para XInput, e os
  jogos reconhecem o controle e a vibração. Sem o Steam Input, a maioria dos jogos de PC não reconhece um
  Pro Controller, **igual ao controle original ligado por cabo**.
- **Giroscópio em emuladores (Eden etc.):** com o Steam aberto, o Steam assume o controle e o emulador vê só um
  controle de Xbox virtual, **sem giroscópio**. Feche o Steam ou desligue o Steam Input para controles do Switch.
- **`joy.cpl` do Windows** mostra botões "piscando": é normal (o Windows não decodifica o relatório do Pro Controller).
- **Acordar o PC:** com o PC suspenso, aperte um botão do controle. A energia USB precisa continuar ligada na
  suspensão (configuração da BIOS). Mais detalhes em [docs/USO.md](docs/USO.md).
- Deixe o dongle longe de portas USB 3.0 e hubs: eles geram interferência em 2,4 GHz e aumentam o jitter do Bluetooth.

## Limitações conhecidas

- **Um controle por dongle** (use dois dongles para dois controles).
- NFC/IR, atualização de firmware do controle e gravação de calibração não são suportados.
- Linux e macOS não foram testados. Clones do Pro Controller não foram testados.
- O dispositivo USB é **composto** (Pro Controller + um teclado HID usado só para acordar o PC) e usa poll de 1 ms,
  então não é um espelho exato do descritor original. Ambos podem ser desligados na compilação
  ([docs/CONFIGURACAO.md](docs/CONFIGURACAO.md)).
- O Bluetooth entrega ~67 relatórios por segundo; o jitter do rádio (algumas vezes 30–45 ms) vem do enlace, não do dongle.

## Compilar a partir do código

Resumo (detalhes em [docs/COMPILAR.md](docs/COMPILAR.md)). Precisa do Pico SDK **2.3.1**, `arm-none-eabi-gcc`,
CMake, Ninja e um compilador C/C++ do PC (para o `pioasm`). No Windows, copie `tools\env.example.cmd` para
`tools\env.cmd`, ajuste os caminhos e rode:

```bat
tools\build.cmd release     rem firmware final   -> firmware\build-release\procontroller_dongle.uf2
tools\build.cmd             rem firmware debug   -> firmware\build\procontroller_dongle.uf2
```

## Documentação

| Documento | Conteúdo |
|---|---|
| [docs/USO.md](docs/USO.md) | Uso detalhado, Windows, Steam, emuladores, wake do PC |
| [docs/COMPILAR.md](docs/COMPILAR.md) | Ferramentas, compilação, gravação, build debug e scripts |
| [docs/CONFIGURACAO.md](docs/CONFIGURACAO.md) | Opções de compilação e comandos da serial (debug) |
| [docs/ARQUITETURA.md](docs/ARQUITETURA.md) | Como o firmware funciona por dentro |
| [docs/PROTOCOLO.md](docs/PROTOCOLO.md) | Protocolo do Pro Controller (Bluetooth e USB) usado aqui |
| [docs/SOLUCAO-DE-PROBLEMAS.md](docs/SOLUCAO-DE-PROBLEMAS.md) | Problemas comuns e como resolver |
| [docs/latencia.md](docs/latencia.md) | Medições de latência |
| [docs/RELEASE.md](docs/RELEASE.md) | Como gerar e publicar uma versão |
| [docs/fase2-resultados.md](docs/fase2-resultados.md) … [fase5](docs/fase5-acabamento.md) | Diário técnico do desenvolvimento |
| [PLANO.md](PLANO.md) | Estado do projeto e pendências |
| [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) | Licenças de terceiros e avisos legais |

## Créditos e licenças

> **Uso pessoal e não comercial.** O **código deste repositório** é [MIT](LICENSE), mas o **firmware compilado (`.uf2`)
> inclui o BTstack (BlueKitchen) e o driver CYW43 (George Robotics)**, cujas licenças permitem uso e redistribuição
> **apenas para fins pessoais e não comerciais**. **Não venda** dongles nem distribua o firmware com fins comerciais sem
> obter as licenças comerciais cabíveis. Detalhes e textos completos em [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).

- O dongle usa o **VID/PID USB da Nintendo (`057E:2009`)** só para ser reconhecido por Steam, SDL e emuladores
  (interoperabilidade), sem autorização da Nintendo nem da USB-IF. Não é adequado a produtos vendidos.
- Protocolo do controle: engenharia reversa da comunidade, principalmente o repositório
  [Nintendo_Switch_Reverse_Engineering](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering) (MIT).
- Inspiração e referência de arquitetura: [DS5Dongle](https://github.com/awalol/DS5Dongle) (MIT). Nenhum código foi copiado.
- Bibliotecas: [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (BSD-3-Clause),
  [TinyUSB](https://github.com/hathach/tinyusb) (MIT), [BTstack](https://github.com/bluekitchen/btstack)
  (licença própria, não comercial) e [cyw43-driver](https://github.com/georgerobotics/cyw43-driver)
  (licença para chips da Raspberry Pi).
