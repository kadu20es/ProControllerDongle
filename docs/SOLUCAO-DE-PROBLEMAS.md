# Solução de problemas

## A unidade `RP2350` não aparece ao segurar o BOOTSEL
- Use um cabo USB **de dados** (muitos cabos só carregam).
- Segure o botão **antes** de plugar e solte depois de plugar. Tente outra porta USB, de preferência traseira
  (direto na placa-mãe, sem hub).
- Confirme que é uma **Raspberry Pi Pico 2 W**. A Pico 2 sem "W" não tem Bluetooth.

## O controle não pareia
- **Remova o controle do Bluetooth do Windows.** Se ele estiver pareado lá, reconecta ao PC e ignora o dongle.
- Segure o **BOOTSEL por 2 s** (o LED deve piscar rápido) e **depois** segure o Sync do controle até as luzes correrem.
- O modo de pareamento do controle dura poucos segundos; o dongle repete a busca sozinho, então comece pelo BOOTSEL e
  logo em seguida o Sync.
- Se o LED não piscar rápido, o BOOTSEL não foi segurado por 2 s inteiros.
- Controles que não são o Pro Controller original (clones) podem se comportar de outra forma.

## O controle não reconecta ao apertar um botão
- Aguarde alguns segundos: a reconexão leva de 2 a 5 s. O LED da placa dá um pulso a cada 2 s enquanto espera.
- Se ele não voltar, refaça o pareamento (BOOTSEL 2 s + Sync).

## O PC não vê o dispositivo / aparece como erro
- Replugue o dongle. Verifique no Gerenciador de Dispositivos o ID `USB\VID_057E&PID_2009`.
- Se o dispositivo ficou travado depois de uma gravação, desplugue, segure BOOTSEL, plugue e regrave.

## O jogo não reconhece o controle
- Sem o **Steam Input**, a maioria dos jogos de PC não reconhece um Pro Controller (igual ao controle original).
  Ligue o Steam Input para o controle ([USO.md](USO.md#steam)) ou adicione o jogo como "jogo não-Steam".

## O giroscópio não funciona no emulador
- O Steam pode estar assumindo o controle. Feche o Steam, ou desligue o Steam Input para controles do Switch.

## O `joy.cpl` mostra botões piscando e eixos loucos
- É normal. O Windows não decodifica o relatório do Pro Controller. Use o Steam, um emulador ou o teste de gamepad de um
  navegador (Chrome/Edge) para ver a entrada correta.

## Não há vibração
- O "Sinalizar" do Steam não gera vibração. Teste num jogo ou em uma página de teste de gamepad do navegador.
- Se nada vibra, repita o pareamento; verifique o nível de bateria do controle.

## O PC não acorda com o controle
- Confirme que a energia USB fica ligada em suspensão (BIOS) e que o Windows permite o wake para a interface do
  teclado do dongle ([USO.md](USO.md#acordar-o-pc-da-suspensão)).
- Aperte o botão e espere de 3 a 5 s (o controle precisa conectar primeiro).
- Se você gravou com `PROCON_WAKE=OFF`, não há wake.

## A latência parece alta ou há travadinhas
- Afaste o dongle de portas USB 3.0, hubs e roteadores 2,4 GHz; use uma extensão USB para colocá-lo à vista do controle.
- O build debug mostra `LAT 5s:` e o `tools\latency-host.ps1` mede a cadência no PC ([latencia.md](latencia.md)).

## `tools\flash.ps1` não grava / diz que a unidade não foi encontrada
- Feche qualquer programa com a porta serial do dongle aberta (como o `serial-log.ps1`).
- O firmware **release** não tem porta serial: use o BOOTSEL manual.

## O build falha
- Confira `PICO_SDK_PATH` (SDK **2.3.1** com submódulos) e se o `arm-none-eabi-gcc`, o Ninja e um compilador C/C++ do PC
  estão acessíveis ([COMPILAR.md](COMPILAR.md)).
- O `tools\build.cmd` imprime `ERRO: defina PICO_SDK_PATH` quando o caminho falta.
