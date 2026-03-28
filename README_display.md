# Display e Pipeline de Vídeo

Este documento descreve como o projeto leva a imagem do Doom até o TFT integrado da `SK-S7G2`.

## Visão geral

O caminho de vídeo usa três blocos principais:

- `doomgeneric` produz um framebuffer de 8 bits por pixel;
- o backend em `src/doomgeneric_renesas.c` copia esse framebuffer para a memória do display;
- o `GLCDC` lê esse buffer em `CLUT8`, enquanto o `ILI9341` atua como controlador do painel.

O resultado é um pipeline enxuto, adequado ao limite de RAM da placa.

## Hardware envolvido

- MCU: `Renesas Synergy S7G2`
- Controlador de display do MCU: `GLCDC`
- Controlador do painel: `ILI9341`
- Interface de comando do painel: `SPI` sobre a instância `g_spi_lcdc`
- Painel integrado: `240 x 320`

## Formato de framebuffer

O projeto usa um único framebuffer de fundo em `CLUT8`, definido na configuração gerada:

- buffer: `g_display_fb_background`
- quantidade: `1`
- resolução alocada: `256 x 320`
- profundidade: `8 bits por pixel`

Pontos importantes:

- a largura alocada é `256`, não `240`;
- o painel visível continua sendo `240 x 320`;
- o stride maior simplifica o layout do `GLCDC` e mantém o alinhamento da memória.

Em termos práticos, o consumo do framebuffer principal é:

- `256 x 320 x 1 byte = 81.920 bytes`

## Arquivos principais do módulo

- `src/lcd_setup.c`
  Inicialização de baixo nível do `ILI9341`.
- `src/lcd.h`
  Pinos e comandos do LCD.
- `src/doomgeneric_renesas.c`
  Conversão do framebuffer do Doom, upload da paleta e overlay dos controles.
- `src/synergy_gen/main_thread.h`
  Declaração do framebuffer e das instâncias do display.
- `src/synergy_gen/main_thread.c`
  Configuração gerada do `GLCDC`.

## Sequência de inicialização do LCD

O `ILI9341` é inicializado por `ILI9341V_Init()` em `src/lcd_setup.c`.

O fluxo principal é:

1. reset físico do painel;
2. `SW_RESET`;
3. programação de registradores de energia, timing, formato de pixel e gama;
4. envio de comandos de modo normal e desativação de inversão;
5. `SLEEP_OUT`;
6. `DISP_ON`.

Os comandos mais relevantes enviados explicitamente no boot são:

- `ILI9341_NORMAL_MODE_ON`
- `ILI9341_DISP_INV_OFF`
- `ILI9341_SLEEP_OUT`
- `ILI9341_DISP_ON`

Essa sequência existe porque o projeto já apresentou casos em que a paleta ou a polaridade aparente do painel ficavam incorretas após reinicializações.

## Inicialização do caminho de display

No backend Renesas:

1. `DG_Init()` chama `renesas_display_init()`;
2. o display gerado pelo SSP é aberto;
3. o LCD é inicializado;
4. o `GLCDC` é iniciado;
5. o framebuffer é limpo;
6. `DG_UpdatePalette()` passa a poder enviar a paleta Doom para a CLUT.

O callback `g_lcd_spi_callback()` sinaliza o fim das transferências SPI usando `g_main_semaphore_lcdc`.

## Como o frame do Doom é desenhado

O Doom trabalha internamente em:

- largura lógica: `320`
- altura lógica: `200`

Já o framebuffer do painel é tratado como:

- painel físico: `240 x 320`
- orientação lógica de uso no port: paisagem para o jogador

Em `DG_DrawFrame()`, cada pixel do framebuffer do Doom é copiado para o framebuffer do display com rotação. O cálculo atual faz:

- `dst_x = DOOM_BORDER_X + y`
- `dst_y = (LCD_PANEL_HEIGHT - 1) - x`

Isso significa:

- a imagem do Doom é rotacionada para caber no painel;
- a altura útil do Doom (`200`) vira a largura visível da área de jogo (`200` colunas);
- surge uma borda lateral calculada por `DOOM_BORDER_X` para centralizar a imagem dentro da largura física de `240`.

## Paleta e CLUT

O Doom produz índices de cor, não pixels RGB diretos. O backend resolve isso com a CLUT do `GLCDC`:

1. `DG_UpdatePalette()` lê a paleta atual de `colors[]`;
2. converte cada entrada para `ARGB8888`;
3. grava as `256` entradas na tabela `g_display_clut_cfg_glcd.p_base`.

Isso reduz drasticamente o uso de RAM em comparação com um framebuffer `RGB565` ou `RGB888`.

## Overlay dos controles

O módulo de vídeo também desenha um overlay simples sobre o framebuffer:

- pads circulares translúcidos;
- marcação visual das áreas de toque;
- sem uso de GUIX.

Esse overlay é produzido diretamente em software, sobre a imagem final do Doom, dentro de `src/doomgeneric_renesas.c`.

## Medidas adicionais de robustez

O port inclui alguns mecanismos práticos para facilitar bring-up e depuração:

- padrão de boot colorido para validar o caminho `GLCDC + CLUT`;
- refresh extra da paleta nos primeiros frames;
- mensagens de estágio em `doom_error_msg`;
- travas explícitas em caso de erro do display.

## Problemas conhecidos

No histórico recente do projeto, os sintomas mais comuns do módulo de display foram:

- tela preta com boot incompleto;
- listras durante a atualização do framebuffer;
- cores invertidas após reinicialização;
- paleta incorreta até o primeiro evento visual relevante.

Nem todos esses sintomas são permanentes em toda execução, mas vale mantê-los documentados porque são típicos de bring-up em hardware embarcado.

## Onde ajustar quando algo der errado

- `src/lcd_setup.c`
  Sequência de comandos do `ILI9341`.
- `src/lcd.h`
  Definições de comandos e pinos do LCD.
- `src/doomgeneric_renesas.c`
  Rotação do frame, overlay, paleta e integração com `GLCDC`.
- `src/synergy_gen/main_thread.h`
  Formato e tamanho do framebuffer.
- `configuration.xml`
  Fonte principal da configuração do display no projeto Synergy.

