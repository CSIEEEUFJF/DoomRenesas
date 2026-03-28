# Memória e Restrições Embarcadas

Este projeto foi moldado pelas restrições reais da `SK-S7G2`. A documentação deste módulo existe para explicar por que várias decisões aparentemente “estranhas” são, na verdade, obrigatórias para o port caber e inicializar.

## Resumo das restrições

- RAM interna disponível: aproximadamente `640 KiB`
- SDRAM externa: **não existe**
- armazenamento de assets: flash QSPI da placa
- sistema operacional: `ThreadX`
- display: exige framebuffer residente em RAM interna

Em outras palavras: o orçamento de memória é pequeno, compartilhado entre RTOS, stacks, heap do BSP, zona do Doom, framebuffer, estruturas do renderer e variáveis globais.

## Estratégia geral do projeto

Para caber na placa, o port adota estas decisões:

- usa um único framebuffer em `CLUT8`;
- evita framebuffers de 16 ou 32 bits;
- mantém o `doom1.wad` fora da RAM de boot, em `.qspi_flash`;
- sobe o jogo com uma linha de comando mínima;
- mantém som e música desativados;
- reduz dependências de ambiente desktop;
- usa `doom_error_msg` como ferramenta de observabilidade barata.

## Números importantes do projeto

Alguns valores atualmente relevantes no repositório:

- heap do BSP: `0x52000`
- stack principal do BSP: `0x800`
- zona padrão do Doom no alvo Renesas: `240 KiB`
- zona mínima de fallback: `176 KiB`
- framebuffer principal do display: `256 x 320 x 1 byte = 81.920 bytes`

Esses números não são independentes. Se um deles crescer demais, outro subsistema pode deixar de inicializar.

## Heap do BSP e zona do Doom

O Doom usa seu próprio alocador de zona, mas essa zona ainda precisa nascer de alguma memória alocada pelo sistema.

No port atual:

- o BSP reserva um heap maior para tornar a alocação inicial viável;
- `I_ZoneBase()` tenta alocar a zona com perfil Renesas em KiB;
- o valor padrão é `240 KiB`;
- se a alocação falhar, o código tenta reduzir o tamanho até o mínimo aceitável.

Essa lógica fica em `src/doomgeneric/i_system.c`.

## WAD embutido em QSPI

O `doom1.wad` convertido para vetor C está em:

- `src/doom1_wad.c`

Ele é colocado na seção:

- `.qspi_flash`

Isso é crucial, porque impede que o conteúdo inteiro do WAD seja copiado para RAM no boot. O acesso ao arquivo é adaptado em `src/doomgeneric/w_file_stdc.c`, que reconhece `doom1.wad` e redireciona a abertura para a implementação embutida.

## Por que o display precisa de tanto cuidado

Em uma placa com pouca RAM, um framebuffer “normal” custa caro. Por isso o projeto usa:

- um único buffer;
- `CLUT8`;
- paleta Doom carregada na CLUT do `GLCDC`.

Essa escolha reduz o custo da camada de vídeo e deixa mais espaço para a zona do Doom e para o renderer.

## Onde normalmente a memória aperta

Os pontos historicamente mais sensíveis foram:

- alocação inicial da zona do Doom;
- tabelas e caches do renderer;
- estruturas relacionadas a texturas e lumps;
- buffers de vídeo e efeitos de transição;
- cópias desnecessárias do WAD para RAM.

Por isso, neste port, várias decisões foram tomadas com foco em footprint, não em luxo de arquitetura.

## Papel de `doom_error_msg`

`doom_error_msg` existe para tornar falhas de memória menos opacas.

Quando a alocação falha, o projeto pode registrar:

- tamanho solicitado;
- memória livre observada;
- tamanho da zona;
- em muitos casos, arquivo e linha da falha.

Isso reduz muito o tempo de diagnóstico em comparação com um travamento “mudo”.

## Sinais clássicos de problema de memória

Se algo voltar a quebrar após mudanças no projeto, preste atenção em mensagens como:

- `Unable to allocate ... for zone`
- `Couldn't realloc lumpinfo`
- `Z_Malloc failed on allocation of ...`

Essas mensagens costumam apontar diretamente para o subsistema que ficou caro demais.

## Regras práticas para futuras mudanças

Antes de adicionar um novo recurso, vale passar por esta lista:

1. Ele exige framebuffer extra?
2. Ele aumenta o número de tabelas residentes?
3. Ele faz cópia de dados grandes para RAM?
4. Ele amplia a zona do Doom?
5. Ele depende de periféricos ou bibliotecas que trazem buffers adicionais?

Se a resposta for “sim” para qualquer item, confira o impacto no `.map` antes de considerar a mudança estável.

## Onde revisar quando o orçamento sair do controle

- `synergy_cfg/ssp_cfg/bsp/bsp_cfg.h`
  Heap e stack do BSP.
- `src/doomgeneric/i_system.c`
  Política de alocação da zona.
- `src/doom1_wad.c`
  Asset embutido.
- `src/doomgeneric/w_file_stdc.c`
  Acesso ao WAD embutido.
- `src/synergy_gen/main_thread.h`
  Definição do framebuffer.
- `Debug/DoomRenesas.map`
  Evidência final do uso de memória após o link.

