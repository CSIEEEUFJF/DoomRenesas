# Build, Gravação e Depuração

Este documento resume o fluxo recomendado para compilar, gravar e depurar o projeto no `e2 studio`.

## Pré-requisitos

- `Renesas e2 studio` com suporte a projetos Synergy;
- toolchain `GNU Arm Embedded` compatível com o projeto;
- placa `SK-S7G2`;
- depurador configurado no ambiente do e2 studio.

## Arquivos que mais influenciam o build

- `configuration.xml`
  Configuração principal do projeto Synergy.
- `src/synergy_gen`
  Código gerado pelo configurador.
- `synergy_cfg/ssp_cfg`
  Configuração do BSP, heap, stacks e parâmetros de módulos.
- `src`
  Código-fonte do port e da engine.
- `script/r7fs7g27h3a01cfc.ld`
  Script de linkedição.

## Fluxo recomendado no e2 studio

1. abra o projeto `DoomRenesas`;
2. confirme se o `configuration.xml` está coerente com o código gerado;
3. evite regenerar os arquivos do Synergy sem revisar depois display, touch, `IIC2`, SPI e pinos;
4. selecione a configuração `Debug`;
5. faça o build do projeto;
6. grave a aplicação na placa;
7. abra uma sessão de debug, se necessário.

## Artefatos gerados

Na configuração `Debug`, o build gera tipicamente:

- `Debug/DoomRenesas.elf`
- `Debug/DoomRenesas.srec`
- `Debug/DoomRenesas.map`

O `.elf` é o principal artefato para depuração. O `.srec` é útil para gravação. O `.map` é essencial quando surgem dúvidas sobre ocupação de memória.

## Build por linha de comando

O projeto também expõe a infraestrutura de `make` dentro da pasta `Debug`. Em um ambiente configurado pelo e2 studio, o build costuma seguir o padrão:

```powershell
make -r -j16 all
```

Mesmo assim, para este projeto, o fluxo mais seguro continua sendo o do próprio `e2 studio`, porque a integração com os arquivos gerados do Synergy precisa permanecer consistente.

## WAD embutido

O projeto espera que o `doom1.wad` já esteja convertido para vetor C em:

- `src/doom1_wad.c`

Esse arquivo é tratado como asset embutido. Portanto:

- não há dependência de sistema de arquivos em tempo de execução para esse WAD;
- mudanças no conteúdo do WAD exigem rebuild do firmware.

## Fluxo de depuração recomendado

Quando o jogo não sobe corretamente:

1. inicie o debug;
2. pare a CPU;
3. inspecione `doom_error_msg`;
4. use essa mensagem para identificar se a falha foi:
   - de boot;
   - de display;
   - de touch;
   - de alocação;
   - de fault da CPU.

## Pontos de breakpoint úteis

- `main_thread_entry()`
- `doomgeneric_Create()`
- `DG_Init()`
- `DG_UpdatePalette()`
- `DG_DrawFrame()`
- `doom_set_error_literal()`
- `doom_set_error_msg()`

## Como interpretar `doom_error_msg`

`doom_error_msg` é o centro da estratégia de depuração do projeto.

Ele pode registrar:

- nomes de estágio, como `stage: ...`;
- falhas de alocação da zona do Doom;
- erros do LCD, SPI ou touch;
- faults de CPU, como `HardFault` e `BusFault`.

Como a variável fica em `.noinit`, ela também ajuda em cenários de falha em que o programa não consegue fazer logging mais elaborado.

## Cuidados com arquivos gerados

Uma observação importante deste projeto:

- vários componentes críticos dependem de `src/synergy_gen` e `synergy_cfg/ssp_cfg`;
- uma regeneração pode sobrescrever callbacks, vetores e configurações finas;
- depois de regenerar, revise pelo menos:
  - `main_thread.c/.h`
  - `common_data.c/.h`
  - `pin_data.c`
  - `bsp_cfg.h`

## Quando olhar o arquivo `.map`

Abra `Debug/DoomRenesas.map` quando precisar confirmar:

- uso de `.bss`;
- uso do heap;
- tamanho do framebuffer;
- posicionamento do WAD embutido;
- crescimento inesperado de memória após alguma mudança.

