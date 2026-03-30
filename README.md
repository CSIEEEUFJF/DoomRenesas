# Port de Doom para uma plataforma de microcontrolador Renesas Synergy
English version available at [README_EN](./README_EN.md).  

Este projeto é um port de [Doom](https://en.wikipedia.org/wiki/Doom_(1993_video_game)) para o microcontrolador Renesas Synergy S7G2. O objetivo do projeto é executar Doom usando o display TFT integrado da placa, sem SDRAM externa, com o arquivo principal do jogo embutido em flash QSPI e com um backend próprio para vídeo, touch e diagnóstico.

O projeto foi ajustado para o perfil real da placa: pouca RAM interna, display controlado por `GLCDC + ILI9341`, inicialização via `ThreadX` e integração direta com os arquivos gerados pelo ecossistema Synergy.

## Documentação por módulo

- [Visão geral do projeto](./README.md)
- [Display e pipeline de vídeo](./README_display.md)
- [Touch, botões e entrada](./README_touch.md)
- [Build, gravação e depuração](./README_build.md)
- [Memória e restrições embarcadas](./README_memory.md)

## Objetivos do port

- Rodar Doom no display TFT integrado da `SK-S7G2`.
- Reaproveitar a engine `doomgeneric` como base do jogo.
- Armazenar o `doom1.wad` em flash QSPI, evitando consumo de RAM no boot.
- Operar sem SDRAM externa.
- Fornecer um caminho de depuração simples por meio da variável global `doom_error_msg`.

## Arquitetura resumida

O projeto é dividido em quatro camadas principais:

1. **Inicialização da placa e RTOS**
   Arquivos gerados pelo Synergy e pelo SSP, como `src/synergy_gen`, `synergy_cfg/ssp_cfg` e `configuration.xml`, configuram `ThreadX`, `GLCDC`, `SCI/SPI`, `RIIC`, `DTC`, pinos e heap do BSP.

2. **Backend Renesas do port**
   O arquivo `src/doomgeneric_renesas.c` implementa a interface esperada pela `doomgeneric`:
   - `DG_Init()`
   - `DG_DrawFrame()`
   - `DG_GetKey()`
   - `DG_SleepMs()`
   - `DG_GetTicksMs()`
   - `DG_UpdatePalette()`

3. **Inicialização e controle do LCD**
   Os arquivos `src/lcd_setup.c` e `src/lcd.h` configuram o `ILI9341` via SPI e deixam o painel pronto para receber o framebuffer produzido pelo `GLCDC`.

4. **Engine Doom**
   O diretório `src/doomgeneric` contém a base principal do Doom. O ponto de entrada do jogo é a thread principal em `src/main_thread_entry.c`.

## Fluxo de execução

O fluxo atual de boot é este:

1. O projeto sobe o `ThreadX` e inicializa as instâncias geradas pelo Synergy.
2. `main_thread_entry()` limpa `doom_error_msg` e chama `doomgeneric_Create()`.
3. `DG_Init()` inicializa display e touch.
4. `doomgeneric_Create()` faz o boot do Doom com uma linha de comando reduzida.
5. O loop infinito chama `doomgeneric_Tick()`.
6. Cada frame passa pelo renderer do Doom e chega ao framebuffer do `GLCDC`.

## Linha de comando atual do jogo

O jogo sobe diretamente com um perfil simplificado, definido em `src/main_thread_entry.c`:

- `-iwad doom1.wad`
- `-skill 1`
- `-warp 1 1`
- `-nomonsters`
- `-nosound`
- `-nomusic`
- `-nogui`

Esse perfil foi escolhido para reduzir carga de CPU, RAM e dependências externas durante a validação do port.

## Estrutura relevante do repositório

- `src/main_thread_entry.c`
  Ponto de entrada da aplicação.
- `src/doomgeneric_renesas.c`
  Backend Renesas para vídeo, touch, botões e temporização.
- `src/lcd_setup.c`
  Sequência de inicialização do `ILI9341`.
- `src/lcd.h`
  Comandos e pinos do LCD.
- `src/doom_port_status.c`
  Diagnóstico global via `doom_error_msg`.
- `src/doom1_wad.c`
  WAD embutido em forma de vetor C.
- `src/doom_embedded_wad.h`
  Declaração do WAD embutido.
- `src/doomgeneric`
  Código-fonte principal do Doom.
- `src/synergy_gen`
  Código gerado do projeto Synergy.
- `synergy_cfg/ssp_cfg`
  Configuração do BSP e de módulos do SSP.
- `configuration.xml`
  Configuração central do projeto no e2 studio/Synergy.
- `script/r7fs7g27h3a01cfc.ld`
  Script de linkedição da aplicação.

## Estado atual

No estado atual do repositório:

- o jogo já inicializa e renderiza no display integrado;
- o `doom1.wad` é carregado a partir de flash QSPI;
- `S4` e `S5` funcionam como disparo;
- o touch já responde, mas ainda requer ajuste fino de geometria e calibração para ficar ideal em todas as direções;
- `doom_error_msg` é o ponto principal de observação quando algo falha no boot ou durante a execução.

## Diagnóstico rápido

Se a placa travar, exibir tela preta ou deixar de responder:

1. pause a execução no debugger;
2. inspecione `doom_error_msg`;
3. confirme em que etapa o boot parou;
4. siga para a documentação temática correspondente.

Os erros mais úteis normalmente aparecem como:

- estágio de boot, por exemplo `stage: ...`;
- falhas de alocação de memória;
- falhas de touch/I2C;
- `HardFault`, `BusFault`, `MemManage` ou `UsageFault`.

## Limitações e observações importantes

- A placa **não possui SDRAM externa**.
- O port depende fortemente de equilíbrio entre framebuffer, heap do BSP e zona do Doom.
- Parte do projeto vive em arquivos gerados pelo Synergy; uma regeneração descuidada pode sobrescrever ajustes importantes do display, do touch e dos periféricos.
- O touch da placa é tratado aqui como caminho de entrada de baixo nível, sem usar toda a pilha gráfica do GUIX.

## Próximos pontos naturais de evolução

- finalizar a calibração do touch e dos pads na orientação final da tela;
- revisar o problema residual de inversão de cores após determinados reinícios;
- estudar melhorias graduais de jogabilidade, como refinamento do input e eventual ajuste de desempenho do renderer.

