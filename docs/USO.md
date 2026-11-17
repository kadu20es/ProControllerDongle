# Uso detalhado

## Visão geral do comportamento

- Ao ligar, o dongle procura o pareamento salvo na flash.
  - **Há pareamento:** não faz busca. Fica aguardando o controle e também tenta conectar ao endereço salvo a cada 4 s.
    Basta apertar um botão do controle.
  - **Não há pareamento:** entra em modo de pareamento (LED pisca rápido) e procura um gamepad por Bluetooth.
- Ao conectar, o dongle configura o controle (lê a SPI, liga IMU e vibração, modo de relatório completo) e liga o
  fluxo USB. O PC passa a ver o Pro Controller em ação.
- Se o controle desconectar, o dongle envia um relatório "solto" (botões em zero, sticks no centro) para que nenhum
  botão fique preso no PC, e volta a aguardar.

## Pareamento

1. Remova o controle da lista de Bluetooth do Windows. Um controle pareado ao Windows reconecta ao PC e não ao dongle.
2. **BOOTSEL por 2 s** (LED pisca rápido). Isso **apaga o pareamento atual** e abre a busca.
3. **Sync** no controle (botão pequeno no topo, perto da entrada USB-C) até as luzes de jogador correrem.
4. O dongle encontra o gamepad, faz o pareamento e conecta. LED fixo = conectado.

Para trocar de controle, repita o processo. Para o log do pareamento, use o firmware debug
([COMPILAR.md](COMPILAR.md)).

## Windows

- O dongle aparece como **"Dispositivo de Entrada USB" / "Controlador de jogo compatível com HID"**
  com ID `USB\VID_057E&PID_2009`. No firmware release não há porta COM.
- O relatório do Pro Controller (`0x30`) **não é descrito** pelo descritor HID genérico. Por isso o `joy.cpl`
  ("Configurar controladores de jogo USB") mostra botões piscando e eixos instáveis. Isso também acontece com o
  controle original. Programas como Steam, SDL, Chrome (Gamepad API) e emuladores leem o formato correto.

## Steam

- Com o **Steam Input** ligado para o controle, o Steam assume o Pro Controller e o apresenta aos jogos como um
  controle de Xbox virtual. É o que faz os jogos reconhecerem o controle e a vibração funcionar.
- Sem o Steam Input, só jogos que implementam o Pro Controller por conta própria o reconhecem. A maioria dos jogos
  de PC usa XInput e não vê o dispositivo. Isso é igual ao controle original por cabo.
- O botão "Identificar controle > Sinalizar" do Steam **não** gera vibração com este controle (o Steam não envia
  o comando); a vibração dos jogos funciona.

## Emuladores (Eden, Dolphin, Ryujinx etc.)

- Usam SDL/HID e leem o Pro Controller direto, com **giroscópio** e vibração.
- **Com o Steam aberto**, o Steam pode assumir o controle e esconder o giroscópio do emulador (ele vê um controle de
  Xbox virtual). Feche o Steam, ou em Steam > Configurações > Controle desligue o Steam Input para controles do
  Switch, ou desative o Steam Input para o item do emulador nas propriedades dele.
- A "caixa" de movimento do emulador pode parecer inclinada na sua mão: o corpo do Pro Controller fica inclinado
  em uma pegada normal. Em uma mesa, deve ficar reta (o dongle não altera os dados do IMU).

## Acordar o PC da suspensão

- O dongle inclui um **teclado HID** mínimo (só envia F15). O Windows só habilita "ativar o computador" para
  dispositivos com aparência de teclado/mouse, e habilita sozinho para essa interface.
- Com o PC em suspensão (S3), apertar um botão do controle faz o controle conectar ao dongle por Bluetooth, e o
  dongle manda o sinal de **USB remote wakeup**. Depois de acordar, ele envia F15 para acender a tela.
- **Requisitos:** a energia USB deve ficar ligada em suspensão (opção da BIOS, às vezes "ErP"/"USB power in S5/S3")
  e o Windows deve permitir o wake. Verifique em um PowerShell:
  `Get-CimInstance -Namespace root\wmi -ClassName MSPower_DeviceWakeEnable | ? InstanceName -match VID_057E`
  (a interface do teclado, `MI_01`, deve estar `Enable = True`).
- Notebooks com "Modern Standby" (S0ix) podem se comportar de forma diferente. Foi testado em um desktop com S3.
- Para desligar o recurso: compile com `-DPROCON_WAKE=OFF` ([CONFIGURACAO.md](CONFIGURACAO.md)).

## Luz do botão Home

O dongle controla a luz: fixa e fraca (~20%) quando conectado, e piscando lentamente (~3,5 s por ciclo) quando a
bateria está crítica e o controle fora da tomada. O controle informa só 5 níveis de bateria (cheio, médio, baixo,
crítico, vazio), não uma porcentagem; "crítico" corresponde a cerca de 15% ou menos.

Os comandos de luz do Home enviados pelo PC (o Eden, por exemplo, manda "apagar" ao iniciar) são **ignorados**.
Para repassá-los ao controle, compile com `-DPROCON_HOME_PASSTHROUGH=ON`.

## Vários controles

Cada dongle atende **um** controle. Para dois controles, use dois dongles (duas Pico 2 W). Cada um aparece como um
Pro Controller separado no PC.
