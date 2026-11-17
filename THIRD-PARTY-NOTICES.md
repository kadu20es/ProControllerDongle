# Avisos de terceiros e licenças

O código-fonte **deste repositório** (`firmware/src`, `tools`, `docs`) é distribuído sob a licença [MIT](LICENSE).

O **firmware compilado** (`.uf2`) **inclui bibliotecas de terceiros**, cada uma com a sua licença. Em especial, o
**BTstack** e o **driver CYW43** têm termos que **restringem o uso comercial** (veja abaixo). Por isso, o arquivo
`.uf2` **não** pode ser tratado como "MIT puro": **o uso e a redistribuição do firmware compilado são permitidos
apenas para fins pessoais e não comerciais**, a menos que você obtenha licenças comerciais dos titulares.

> Este documento é um resumo técnico feito de boa-fé a partir dos arquivos de licença incluídos nas bibliotecas. Não é
> aconselhamento jurídico. Se você pretende **vender** dongles ou distribuir o firmware com fins comerciais, consulte
> um advogado e obtenha as licenças comerciais cabíveis.

## O que entra no firmware

| Componente | Versão usada | Licença | Restrição relevante |
|---|---|---|---|
| Código deste projeto | 1.0.0 | MIT | Mantenha o aviso de copyright |
| Raspberry Pi **Pico SDK** | 2.3.1 | BSD-3-Clause | Reproduzir o aviso e a isenção nos materiais de uma redistribuição binária; não usar o nome dos autores para promoção |
| **TinyUSB** | commit `86ad6e5` (do SDK 2.3.1) | MIT | Manter o aviso de copyright |
| **BTstack** (BlueKitchen) | commit `eb0bb8b` (do SDK 2.3.1) | BSD com cláusula 4 | **Uso, modificação e redistribuição somente para benefício pessoal, sem fim comercial ou ganho monetário.** Uso comercial exige licença da BlueKitchen (contact@bluekitchen-gmbh.com) |
| **cyw43-driver** (George Robotics) | commit `055d642` (do SDK 2.3.1) | `LICENSE.RP` (uso com chips da Raspberry Pi) | Só pode ser usado e redistribuído em conjunto com chips da Raspberry Pi (o RP2350 da Pico 2 W atende). A licença geral do driver (`LICENSE`) é não comercial |
| Firmware do chip **CYW43439** (blobs de Wi-Fi e Bluetooth embutidos no driver) | do cyw43-driver | Proprietário (Infineon/Cypress), distribuído pela Raspberry Pi junto ao SDK | **Não consegui confirmar o texto exato da licença** dos blobs. Eles são redistribuídos pelo Pico SDK e por todos os projetos para Pico W; não os modifique |
| micro-ecc, md5, rijndael, yxml, SEGGER RTT (dentro do BTstack) | do BTstack | BSD-2, domínio público, domínio público, MIT, BSD-3 | Manter os avisos |
| newlib / libgcc (runtime da toolchain GCC ARM) | da toolchain (xPack GCC 15.2.1) | BSD e variantes; libgcc com *runtime library exception* | Sem restrição prática para o binário |

O firmware **não** inclui lwIP nem mbedtls (não aparecem no arquivo `.map` do build release).

## Outros pontos legais do projeto

- **VID/PID USB `057E:2009` (Nintendo).** O dongle se apresenta com o identificador de fornecedor e de produto do Nintendo
  Switch Pro Controller para que Steam, SDL e emuladores o reconheçam. Isso é feito **só por interoperabilidade**, sem
  autorização da Nintendo nem da USB-IF (que atribui os VIDs). **Não use o firmware em produtos vendidos** sem obter
  um VID/PID próprio e adaptar os drivers de reconhecimento.
- **Marcas.** "Nintendo", "Switch" e "Pro Controller" são marcas dos seus titulares. Este projeto é independente e
  não tem vínculo com a Nintendo.
- **Descritores USB** do Pro Controller foram copiados de um dump público do dispositivo original, apenas para
  compatibilidade funcional.
- **Protocolo.** O protocolo do controle vem de engenharia reversa da comunidade
  ([dekuNukem/Nintendo_Switch_Reverse_Engineering](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering),
  licença MIT). Foram usados apenas fatos de protocolo; nenhum código foi copiado.
- **DS5Dongle** ([awalol/DS5Dongle](https://github.com/awalol/DS5Dongle), MIT) serviu de inspiração e referência de
  arquitetura; nenhum código foi copiado.

## Se você publica binários (checklist)
1. Inclua este arquivo (`THIRD-PARTY-NOTICES.md`) junto do `.uf2`, como o release faz.
2. Deixe claro que o uso é **pessoal e não comercial** por causa do BTstack e do driver CYW43.
3. Não remova os avisos de copyright dos componentes.

---

# Textos das licenças

Os textos abaixo foram copiados dos arquivos de licença das bibliotecas, nas versões usadas.


## Raspberry Pi Pico SDK 2.3.1 (BSD-3-Clause)

```text
Copyright 2020 (c) 2020 Raspberry Pi (Trading) Ltd.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
   disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```


## TinyUSB (MIT)

```text
The MIT License (MIT)

Copyright (c) 2018, hathach (tinyusb.org)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```


## BTstack — BlueKitchen GmbH

Atenção à cláusula 4: uso, modificação e redistribuição apenas para benefício pessoal, sem fim comercial.

```text
Copyright (C) 2009 BlueKitchen GmbH
All rights reserved 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

4. Any redistribution, use, or modification is done solely for
   personal benefit and not for any commercial purpose or for
   monetary gain.

THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN 
GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

Please inquire about commercial licensing options at 
contact@bluekitchen-gmbh.com
```


## cyw43-driver — George Robotics (LICENSE.RP, para chips da Raspberry Pi)

O texto geral do driver (`LICENSE`, não comercial) está em `lib/cyw43-driver/LICENSE` no Pico SDK.

```text
Copyright (C) 2019-2022 George Robotics Pty Ltd

Raspberry Pi Ltd (Licensor) hereby grants to you a non-exclusive license to
use this software solely with the Licensor's microcontroller chip (RP2040) or
any other semiconductor device produced by the Licensor. No other use is
permitted under the terms of this licence.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. The software can only be used and redistributed in conjunction with RP2040
   or any other semiconductor device produced by the Licensor.
2. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
3. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE LICENSOR AND COPYRIGHT OWNER "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE LICENSOR OR COPYRIGHT OWNER BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This software is also available from the copyright owner under different
terms (see ./LICENSE)
```


## micro-ecc (dentro do BTstack)

```text
Copyright (c) 2014, Kenneth MacKay
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```


## yxml (dentro do BTstack)

```text
Copyright (c) 2013-2014 Yoran Heling

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```


## SEGGER RTT (dentro do BTstack)

```text
Important - Read carefully:

SEGGER RTT - Real Time Transfer for embedded targets

All rights reserved.

SEGGER strongly recommends to not make any changes
to or modify the source code of this software in order to stay
compatible with the RTT protocol and J-Link.

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following
conditions are met:

o Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

o Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.

o Neither the name of SEGGER Microcontroller GmbH
  nor the names of its contributors may be used to endorse or
  promote products derived from this software without specific
  prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.


(c) 2014 - 2016  SEGGER Microcontroller GmbH
www.segger.com
```


## md5 e rijndael (dentro do BTstack)

Código em domínio público (ver `lib/btstack/3rd-party/md5` e `rijndael` no Pico SDK).
