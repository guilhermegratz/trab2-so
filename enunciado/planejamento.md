# Plano de Implementação — sim-virtual (Simulador de Memória Virtual)

## Estrutura de arquivos

- **main.c** — parsing de argumentos de linha de comando, leitura do arquivo de log para um vetor de acessos, loop principal de simulação, impressão do relatório final de saída (stdout).
- **memsim.h** — definições de structs (PageTableEntry, Frame, Stats), constantes (incluindo `#define` do período de reset do bit R do NRU), protótipos de funções comuns.
- **memsim.c** — funções de tradução de endereço lógico→página, controle da tabela de páginas (array), controle dos quadros (array), contabilização de estatísticas (page faults, escritas), lógica de "instalar página em quadro" comum a todos os algoritmos.
- **alg_lru.c / alg_lru.h** — implementação específica do algoritmo LRU.
- **alg_nru.c / alg_nru.h** — implementação específica do NRU (incluindo o reset periódico do bit R, controlado pelo `#define`).
- **alg_clock.c / alg_clock.h** — implementação do algoritmo do Relógio (ponteiro circular).
- **alg_otimo.c / alg_otimo.h** — implementação do algoritmo Ótimo (necessita acesso ao vetor completo de acessos futuros).
- **algoritmo.h** — interface comum (struct com ponteiros de função, ex.: `escolher_pagina_vitima()`, `notificar_acesso()`), para que main.c chame o algoritmo escolhido de forma genérica (padrão "strategy").

## Funções principais (por módulo)

### main.c
- `parse_args(argc, argv, &config)` — valida e converte os 4 parâmetros (algoritmo, arquivo, tamanho de página, tamanho de memória).
- `carregar_log(filename) -> vetor de acessos` — lê todas as linhas com fscanf, armazena endereço + flag R/W em um array dinâmico (necessário para o Ótimo, e simplifica os demais).
- `simular(vetor_acessos, n, config)` — loop principal: para cada acesso, calcula página, verifica presença, trata fault se necessário, atualiza bits R/M, incrementa `time`.
- `imprimir_relatorio(config, stats)` — imprime no formato de saída especificado no enunciado.

### memsim.h / memsim.c
- `calcular_bits_offset(tamanho_pagina_KB) -> s` — calcula quantos bits de deslocamento usar (suporta 4KB e 8KB).
- `obter_pagina(addr, s) -> page = addr >> s`.
- `inicializar_tabela_paginas(num_paginas_max)` — array estático/dinâmico simples.
- `inicializar_quadros(num_quadros)` — array de structs `Frame`.
- `buscar_quadro_livre() -> int` — percorre o array de quadros e retorna índice de quadro livre ou -1.
- `instalar_pagina(pagina, quadro, time)` — atualiza tabela de páginas e estrutura do quadro (R, M, timestamps).
- `tratar_acesso(pagina, rw, time) -> fault?` — função central chamada pelo loop de simulação:
  - se página presente: atualiza R (sempre) e M (se escrita), retorna sem fault.
  - se página ausente:
    - conta page fault.
    - se houver quadro livre (memória ainda não cheia): instala diretamente, **sem acionar o algoritmo de substituição**.
    - se não houver quadro livre: aciona `escolher_pagina_vitima()` do algoritmo configurado, escreve a página vítima de volta (se M=1, conta como página suja escrita), instala a nova página no quadro liberado.

### algoritmo.h (interface comum)
- `typedef struct { int (*escolher_vitima)(...); void (*notificar_acesso)(...); ... } AlgoritmoSubstituicao;`
- Permite que `main.c`/`memsim.c` chamem o algoritmo escolhido via ponteiros de função, sem `if/else` espalhado pelo código.

### alg_lru.c / alg_lru.h
- `lru_escolher_vitima(frames, num_frames) -> indice_do_quadro` — percorre os quadros ocupados e retorna o de menor timestamp de último acesso.
- `lru_notificar_acesso(quadro, time)` — atualiza timestamp do quadro acessado.

### alg_nru.c / alg_nru.h
- `#define NRU_RESET_INTERVAL <valor>` em `memsim.h` (ou `alg_nru.h`), facilmente ajustável.
- `nru_escolher_vitima(frames, num_frames) -> indice_do_quadro` — classifica páginas em 4 categorias (R=0,M=0 / R=0,M=1 / R=1,M=0 / R=1,M=1) e escolhe a primeira encontrada na categoria mais baixa.
- `nru_tick(time)` — a cada `NRU_RESET_INTERVAL` acessos processados, zera o bit R de todos os quadros ocupados.

### alg_clock.c / alg_clock.h
- `clock_escolher_vitima(frames, num_frames, &ponteiro) -> indice_do_quadro` — mantém um ponteiro circular sobre os quadros; avança verificando R; se R=1, zera e avança; se R=0, substitui.

### alg_otimo.c / alg_otimo.h
- `otimo_precalcular_proximos_acessos(vetor_acessos, n)` — pré-processa, para cada posição do vetor de acessos, a próxima ocorrência futura de cada página (estrutura auxiliar de apoio), de forma a evitar varredura completa do vetor a cada fault.
- `otimo_escolher_vitima(frames, num_frames, posicao_atual) -> indice_do_quadro` — entre as páginas presentes, escolhe a que tem o próximo acesso mais distante (ou nunca mais acessada).

## Subtarefas (ordem de execução)

**1. Parsing de argumentos e leitura do arquivo de log**
- Dependências: nenhuma.
- Prioridade: 1.
- Objetivo: obter de forma confiável os 4 parâmetros e o vetor de acessos em memória (necessário inclusive para o algoritmo Ótimo).

**2. Definição das estruturas de dados (tabela de páginas, quadros, parâmetros de tamanho)**
- Dependências: depende de 1 (precisa saber tamanho de página para calcular `s` e número de páginas possíveis).
- Prioridade: 2.
- Objetivo: representar o estado da memória usando arrays simples: array de quadros (`Frame[]`, contendo página alocada, bit R, bit M, timestamp de último acesso, timestamp de carga) e array para a tabela de páginas (presente/ausente, número do quadro).

**3. Tradução de endereço lógico para página + cálculo de `s` dinâmico**
- Dependências: depende de 1 e 2.
- Prioridade: 3.
- Objetivo: implementar `page = addr >> s` com `s` calculado a partir do tamanho de página em KB (suporta 4KB e 8KB).

**4. Lógica central de tratamento de acesso (fault ou não, atualização de R/M)**
- Dependências: depende de 2 e 3.
- Prioridade: 4.
- Objetivo: para cada acesso, verificar se a página está presente; se sim, atualizar R (sempre) e M (se for escrita); se não, contar page fault; se houver quadro livre, instalar diretamente sem acionar o algoritmo de substituição; se não houver, acionar o algoritmo configurado.

**5. Implementação do algoritmo LRU**
- Dependências: depende de 4.
- Prioridade: 5.
- Objetivo: escolher como vítima o quadro com o menor timestamp de último acesso.

**6. Implementação do algoritmo NRU**
- Dependências: depende de 4; usa `#define NRU_RESET_INTERVAL` para definir a cada quantas referências o bit R é zerado.
- Prioridade: 6.
- Objetivo: classificar páginas em 4 categorias e escolher vítima da categoria mais baixa disponível; resetar R periodicamente conforme o `#define`.

**7. Implementação do algoritmo do Relógio**
- Dependências: depende de 4.
- Prioridade: 7.
- Objetivo: manter ponteiro circular sobre os quadros; se R=1, zera e avança; se R=0, substitui.

**8. Implementação do algoritmo Ótimo**
- Dependências: depende de 4 e do vetor completo de acessos carregado em memória (subtarefa 1).
- Prioridade: 8 (mais custoso computacionalmente; deixar por último).
- Objetivo: escolher como vítima, entre as páginas presentes, a que tem o próximo acesso futuro mais distante (ou nunca mais ocorre). Pré-computar estrutura de "próxima ocorrência" para evitar varredura completa a cada fault.

**9. Contabilização de estatísticas e geração do relatório textual de saída**
- Dependências: depende de 4-8.
- Prioridade: 9.
- Objetivo: imprimir no formato exigido pelo enunciado (configuração usada, número de page faults, número de páginas escritas).

**10. Execução das simulações para todos os arquivos/parâmetros**
- Dependências: depende de todas as anteriores.
- Prioridade: 10.
- Objetivo: rodar o simulador para os 4 arquivos de log, combinando os tamanhos de página (4KB/8KB) e de memória (1MB/2MB/4MB), para todos os 4 algoritmos.

## Decisões já confirmadas

1. Estruturas de dados implementadas com **arrays simples** (tabela de páginas e array de quadros).
2. Período de reset do bit R no NRU controlado por **`#define`** facilmente ajustável no código.
3. Quando há page fault mas ainda existem quadros livres, **o algoritmo de substituição não é acionado** — a página é instalada diretamente no próximo quadro livre.
4. Considerações sobre o relatório (.pdf) **fora de escopo** neste plano.
