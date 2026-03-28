# Touch, Botões e Entrada

Este documento descreve o caminho de entrada do projeto: touch resistivo, botões físicos e tradução para eventos de teclado da engine Doom.

## Visão geral

O projeto usa dois caminhos de entrada:

- touch integrado da placa, lido diretamente pelo controlador `SX8654`;
- botões físicos `S4` e `S5`.

Em vez de depender da pilha completa de GUIX para input, o port acessa o touch em nível mais baixo, pela interface `RIIC2`, e converte os gestos em teclas que a `doomgeneric` já entende.

## Arquivos principais

- `src/doomgeneric_renesas.c`
  Inicialização do touch, leitura do controlador, calibração e mapeamento para teclas.
- `src/synergy_gen/common_data.c`
  Instância `g_i2c` usada para falar com o `SX8654`.
- `src/synergy_gen/pin_data.c`
  Configuração gerada de pinos.
- `src/doom_port_status.c`
  Registro de falhas e estados em `doom_error_msg`.

## Hardware e sinais usados

- controlador touch: `SX8654`
- barramento: `IIC2`
- endereço configurado: `0x48`
- pino de reset do touch: `TOUCH_RESET_PIN`
- botões físicos de disparo: `S4` e `S5`

## Sequência de inicialização do touch

O touch é inicializado por `renesas_touch_init()` em `src/doomgeneric_renesas.c`.

O fluxo atual é:

1. abrir `g_i2c`;
2. resetar o controlador de touch;
3. configurar registradores de operação;
4. habilitar canais `X/Y`;
5. configurar interrupções de `pendown` e `penrelease`;
6. colocar o controlador em modo de aquisição por caneta.

Se alguma dessas etapas falhar, o touch é desativado e a falha fica registrada em `doom_error_msg`.

## Modelo de entrada usado pelo jogo

O Doom continua recebendo eventos como se estivesse rodando com teclado:

- `KEY_LEFTARROW`
- `KEY_RIGHTARROW`
- `KEY_UPARROW`
- `KEY_DOWNARROW`
- `KEY_STRAFE_L`
- `KEY_STRAFE_R`
- `KEY_FIRE`

O backend Renesas mantém uma pequena fila de eventos (`g_key_queue`) e traduz o estado do touch e dos botões para essas teclas.

## Overlay visual atual

O projeto desenha dois pads circulares sobre a imagem:

- pad esquerdo: movimento e strafe;
- pad direito: rotação da câmera;
- `S4` e `S5`: disparo.

Importante:

- o overlay é apenas a representação visual;
- a lógica real depende do mapeamento de coordenadas em `touch_pad_mask()`;
- o painel é tratado, na prática, como caminho de toque único durante o gameplay.

## Conversão de coordenadas

As coordenadas lidas do `SX8654` são brutas e precisam ser transformadas para o espaço lógico usado pelo jogo.

Hoje o backend faz:

1. leitura de `raw_x` e `raw_y`;
2. expansão gradual da faixa observada;
3. escalonamento para:
   - largura lógica do usuário: `320`
   - altura lógica do usuário: `240`

Os limites iniciais de calibração são conservadores:

- mínimo padrão: `512`
- máximo padrão: `3072`

Esses valores ficam no próprio `src/doomgeneric_renesas.c`.

## Estado atual do módulo

O touch já responde no hardware real, mas ainda está em fase de ajuste fino. Em especial:

- a geometria dos pads ainda pode exigir refinamento;
- a resposta de frente/trás e da câmera pode variar conforme a orientação percebida do painel;
- pequenas mudanças de calibração alteram bastante a sensação de controle.

Por isso, este módulo deve ser tratado como funcional, porém ainda em estabilização.

## Limitações práticas

- O uso é efetivamente **single-touch** para gameplay.
- O comportamento depende da orientação visual final do display.
- Uma regeneração dos arquivos do Synergy pode exigir conferência do `IIC2`, dos pinos e do reset do touch.
- O overlay não usa GUIX, então toda mudança de visual ou geometria precisa ser ajustada no backend do port.

## Botões físicos

Os botões `S4` e `S5` são lidos por GPIO e, quando pressionados, geram `KEY_FIRE`.

Esse caminho é mais simples e mais estável do que o touch, por isso é uma referência útil na depuração do input: se `S4/S5` funcionarem e o touch não, o problema tende a estar no caminho `SX8654 -> I2C -> calibração -> mapeamento`.

## Depuração do input

Se o touch parar de responder ou começar a responder de forma incoerente:

1. verifique `doom_error_msg`;
2. confirme se `g_i2c` abriu corretamente;
3. revise `renesas_touch_init()`;
4. revise `touch_poll_controller()`;
5. ajuste `touch_pad_mask()` e os parâmetros dos pads;
6. confira a orientação lógica usada em `g_touch_state.x` e `g_touch_state.y`.

## Pontos de ajuste mais importantes

Para recalibrar ou mudar o comportamento dos pads, os melhores pontos de entrada são:

- `TOUCH_PAD_LEFT_X`
- `TOUCH_PAD_RIGHT_X`
- `TOUCH_PAD_CENTER_Y`
- `TOUCH_PAD_RADIUS`
- `TOUCH_PAD_DEADZONE`
- `TOUCH_CAL_MIN_DEFAULT`
- `TOUCH_CAL_MAX_DEFAULT`
- função `touch_pad_mask()`
- função `touch_poll_controller()`

