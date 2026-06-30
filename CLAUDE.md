# CLAUDE.md — sim-virtual (Trabalho 2 de Sistemas Operacionais, INF1316)

> Documento de referência para todas as próximas sessões. Consolida o
> entendimento do enunciado, da documentação de apoio, e do estado atual
> do código. **Atualize-o sempre que o estado do projeto mudar.**

---

## Visão Geral

- **Disciplina:** INF1316 — Sistemas Operacionais (PUC-Rio).
- **Trabalho:** 2º trabalho — Gerência de Memória (período 2026-1). Enunciado
  adaptado de trabalho do prof. Dorgival Guedes Neto (DCC/UFMG).
- **Objetivo:** implementar em **C** um simulador de **memória virtual**
  (`sim-virtual`) que realiza paginação (mapeamento lógico → físico) e
  implementa **quatro algoritmos de substituição de páginas**: LRU, NRU,
  Relógio (Clock) e Ótimo (MIN/Belady).
- **Problema resolvido:** dada uma sequência real de acessos à memória de um
  programa (arquivo `.log`), simular a paginação por demanda — detectar
  *page faults*, escolher páginas vítimas quando a memória física está cheia,
  contabilizar escritas de páginas sujas — e gerar um relatório de
  estatísticas. Serve para **comparar o desempenho** dos algoritmos variando
  tamanho de página e tamanho de memória física.

---

## Objetivos do Projeto

### O que precisa ser entregue (entregáveis)

1. **Código-fonte** do simulador (`.c` e `.h`). ✅ *já existe e aparenta estar completo*.
2. **Relatório (.pdf)** sobre o trabalho. ❌ *ainda não iniciado* — peso significativo na nota. Deve conter:
   - Resumo do projeto (estrutura geral + estruturas de dados de cada algoritmo).
   - **Análise de desempenho**: comparar page-faults e escritas mantendo a
     memória física fixa e variando o tamanho de página; comparar tamanhos de
     página; comparar algoritmos e justificar os valores.
   - **Tabela** com os dados de cada simulação (algoritmo, tam. página, tam.
     memória, nº page-faults, nº páginas sujas).
   - **Pseudo-código do algoritmo Ótimo** (descrição do algoritmo ideal).
3. **Execução de TODAS as simulações** e coleta dos dados para a tabela. ❌ *pendente*.
4. **NÃO** incluir os arquivos `.log` no kit de entrega.
5. **Identificar os integrantes do grupo** no cabeçalho do simulador e no
   relatório, e qual algoritmo cada um implementou/executou. ⚠️ *os cabeçalhos
   atuais NÃO listam os integrantes — gap a corrigir.*

### Requisitos obrigatórios

- CLI com **4 argumentos**, nesta ordem:
  `sim-virtual <algoritmo> <arquivo.log> <tam_pagina_KB> <tam_memoria_MB>`
  - `algoritmo`: LRU | NRU | Relogio | Otimo
  - `arquivo.log`: sequência de acessos
  - `tam_pagina_KB`: **4 ou 8**
  - `tam_memoria_MB`: **1, 2 ou 4**
- Cálculo de `s` (bits de offset) **dinâmico** a partir do tamanho de página;
  obter página via `page = addr >> s`.
- Estrutura de quadro físico com flags **R** (referência), **M** (modificação)
  e **instante do último acesso**.
- Detectar page-faults; acionar substituição **somente quando todos os quadros
  estão ocupados**.
- Página vítima suja (M=1) conta como **escrita de volta ao disco**.
- Contador `time` iniciando em 0, incrementado a cada acesso processado.
- Relatório final no formato especificado (ver abaixo).
- Implementar os **4 algoritmos** (LRU, NRU, Relógio, Ótimo).
- Rodar **todas** as combinações: 4 logs × {4,8} KB × {1,2,4} MB × 4 algoritmos.

### Requisitos opcionais

- Não há requisitos explicitamente opcionais. O enunciado menciona que os bits
  de controle além de R/M/last_access "ficam a critério" do implementador
  (liberdade de projeto, não obrigação).
- Apresentação oral/demonstração é uma possibilidade (não obrigatória).

### Critérios de sucesso / avaliação

- Simulador funcional para todas as combinações de parâmetros.
- Corretude dos algoritmos de substituição (especialmente o Ótimo como
  *benchmark*).
- **Relatório com análise de desempenho tem peso significativo na nota.**
- Grupos de **até 4 alunos**.

### Restrições de implementação

- Linguagem **C**.
- Endereços de 32 bits, hexadecimais, seguidos de `R`/`W`.
- Leitura sugerida via `fscanf(file, "%x %c", &addr, &rw)` com `addr` unsigned.
- Tabela de páginas pode ser grande (até ~2^19/2^20 entradas) — aceitável para
  simulação de **um único programa** por vez.

---

## Estrutura do Projeto

```
trab2-so/
├── CLAUDE.md              ← este arquivo
├── main.c                 ← CLI, leitura do log, loop de simulação, relatório
├── memsim.c / memsim.h    ← núcleo: tradução end.→página, tabela de páginas,
│                            quadros, estatísticas, tratar_acesso() comum
├── algoritmo.h            ← interface "strategy" (ponteiros de função)
├── alg_lru.c  / alg_lru.h     ← LRU
├── alg_nru.c  / alg_nru.h     ← NRU
├── alg_clock.c/ alg_clock.h   ← Relógio (Clock / 2ª chance)
├── alg_otimo.c/ alg_otimo.h   ← Ótimo (MIN/Belady)
├── arquivos/              ← logs de entrada (NÃO entregar): compilador, compressor,
│                            matriz, simulador (.log) — 1.000.000 de acessos cada
├── enunciado/
│   ├── Trabalho 2 - gerencia de memoria - 2026-1.pdf
│   └── planejamento.md    ← plano de implementação original (10 subtarefas)
└── docs/                  ← slides do curso (referência teórica)
```

### Papel de cada módulo

| Módulo | Responsabilidade |
|---|---|
| **main.c** | `parse_args` (valida 4 args), `carregar_log` (lê todo o log p/ vetor dinâmico de `Acesso`), cálculo de `s` e da página de cada acesso, inicialização de tabela/quadros, `simular` (loop), `imprimir_relatorio`. |
| **memsim.c/.h** | `calcular_bits_offset`, `inicializar_tabela_paginas`, `inicializar_quadros`, `buscar_quadro_livre`, `instalar_pagina`, **`tratar_acesso`** (lógica central de fault/hit + atualização de R/M + acionamento da substituição), getters de estado, `memsim_set_algoritmo`. Define structs `Acesso`, `PageTableEntry`, `Frame`, `Stats`, `Config`. |
| **algoritmo.h** | `struct AlgoritmoSubstituicao` com ponteiros de função: `inicializar`, `preparar`, `escolher_vitima`, `notificar_acesso`, `notificar_carga`, `tick`. Padrão **strategy** — evita if/else espalhado. |
| **alg_lru.c** | Vítima = quadro de **menor `last_access`**. |
| **alg_nru.c** | Classifica em 4 classes (R,M) e escolhe a classe mais baixa não-vazia; `tick` zera o bit R a cada `NRU_RESET_INTERVAL` acessos. |
| **alg_clock.c** | Ponteiro circular; R=1 → zera e avança (2ª chance), R=0 → vítima. |
| **alg_otimo.c** | `preparar` pré-computa `proximo_acesso[i]` (próxima ocorrência da mesma página) em O(n); vítima = página de próximo acesso mais distante (ou nunca mais). |

---

## Estado Atual

### O que já foi implementado (aparenta estar completo e funcional)

- ✅ Parsing e validação dos 4 argumentos (`main.c:parse_args`).
- ✅ Leitura do log para vetor dinâmico com `realloc` (`carregar_log`).
- ✅ Cálculo dinâmico de `s` para qualquer potência de 2 (`calcular_bits_offset`).
- ✅ Tabela de páginas e array de quadros (`Frame`) com R, M, last_access, load_time.
- ✅ `tratar_acesso`: hit atualiza R (sempre) e M (se escrita); fault conta,
  usa quadro livre se houver, senão aciona o algoritmo; vítima suja conta escrita.
- ✅ **LRU**, **NRU** (com reset periódico de R), **Relógio**, **Ótimo** — todos
  via interface comum `AlgoritmoSubstituicao`.
- ✅ Relatório no formato do enunciado (`imprimir_relatorio`).
- ✅ Aceita variações de grafia: "Relogio"/"Relógio"/"Clock", "Otimo"/"Ótimo"/"OPT".

### O que ainda falta

- ✅ **Build system**: `Makefile` criado (`make`, `make clean`, `make simulacoes`).
- ✅ **Script de simulações**: `run_simulacoes.sh` roda as 96 execuções → `resultados.csv`.
- ✅ **Simulações executadas**: todas as 96 combinações rodadas; dados em
  `resultados.csv` e tabulados em `RELATORIO.md` (§7).
- ✅ **Relatório**: `RELATORIO.md` completo (resumo, algoritmos, pseudo-código do
  Ótimo, tabela e análise de desempenho com dados reais).
- ✅ **Cabeçalhos com os integrantes do grupo** preenchidos em todos os `.c` e no
  `RELATORIO.md`. Integrantes: Rafael Prates (2210234), Thiago Coqueiro e
  Guilherme Gratz. **Atenção:** as matrículas de Thiago Coqueiro e Guilherme
  Gratz estão marcadas como "(matrícula pendente)" — substituir assim que forem
  informadas (todos os 6 arquivos `.c` e o `RELATORIO.md`).
- ⬜ **Gerar `.pdf`** do relatório a partir do `.md`, se a entrega exigir PDF.
- ⬜ **Empacotamento**: remover `arquivos/*.log` e o executável do kit final.

### Pontos que precisam ser validados

1. **Corretude numérica** comparando algoritmos contra o Ótimo (Ótimo deve dar
   ≤ page-faults que os demais) e usando o exemplo manual dos slides 90–92.
2. **NRU_RESET_INTERVAL = 1000** (em `memsim.h`) — valor de projeto, ajustável.
   Confirmar que faz sentido p/ logs de 1M de acessos; documentar a escolha no
   relatório.
3. **Ótimo** usa `proximo_acesso[quadro.last_access]`. Validar a invariante de
   que `time` (índice do loop) coincide com o índice no vetor de acessos —
   verdadeiro hoje porque `simular` usa `time = 0..n-1` (`main.c:simular`).
4. **NRU não reseta o bit M** (correto: o enunciado só fala em zerar R no
   tick do relógio). Confirmar no relatório que páginas sujas finais não são
   contadas como escritas (já tratado em `tratar_acesso`).
5. Comportamento quando um log tem `R`/`W` minúsculo — normalizado em
   `carregar_log` e em `tratar_acesso`.

---

## Mapeamento da Documentação

### Categoria A — Gerência de Memória / Memória Virtual / Page Replacement (FUNDAMENTAL)

| Documento | Localização | Utilidade | Relação com o projeto |
|---|---|---|---|
| **INF1316 - 7 Gerencia de Memoria.pdf** | `docs/` | **Documento central.** 92 slides cobrindo paginação, MMU, tabela de páginas, bits R/M/Present, e TODOS os algoritmos de substituição. Slides-chave: 26–34 (paginação, frame table, entrada da TP), 46–47 (substituição + Ótimo), 48 (NRU + 4 classes), 49–51 (FIFO/2ª chance/**Clock**), 52–55 (**LRU**/aging), 61 (resumo comparativo), 62 (anomalia de Belady), **90–92 (exemplo resolvido LRU vs NRU com reset de R a cada 4 refs — espelha exatamente a lógica do trabalho).** | Base teórica de 100% do simulador. Use para escrever o relatório e justificar resultados. |
| **Trabalho 2 ... 2026-1.pdf** | `enunciado/` | Enunciado oficial. | Define todos os requisitos. |
| **planejamento.md** | `enunciado/` | Plano original em 10 subtarefas + decisões de projeto confirmadas. | Mapeia diretamente para os módulos existentes. |

### Categoria B — Material de curso (REFERÊNCIA / CONTEXTO, não específico deste trabalho)

Os PDFs abaixo em `docs/` são slides de **outros tópicos** da disciplina
(provavelmente de uma oferta anterior, prefixo "INF 1019"/"INF1316"). Não são
necessários para implementar o simulador, mas servem de contexto geral.

| Documento | Tema |
|---|---|
| INF 1019 1 Conceitos Basicos.pdf | Conceitos básicos de SO |
| INF 1019 2 Historico do Unix.pdf | História do Unix |
| INF 1019 3 O sistema operacional Unix.pdf | Unix |
| INF 1019 4 Processos e Escalonamento.pdf | Processos e escalonamento |
| INF1316 - 5 Comunicacao entre processos.pdf | IPC |
| Comunicacao de Processos - memoria compartilhada.pdf | Memória compartilhada |
| INF1316 - 6 Entrada e Saida.pdf | E/S |
| INF1316 - 8 Sistema de Arquivos (1).pdf | Sistema de arquivos |
| INF1316 SO Lab5 sinais 2025-1.pdf | Lab: sinais |
| INF1316 SO Lab 5 Threads 2025-1 slides.pdf | Lab: threads |
| INF1316 SO Lab 7 Semaforos 2025_1.pdf | Lab: semáforos |

> **Conclusão do mapa de conhecimento:** apenas **1 documento** (`INF1316 - 7
> Gerencia de Memoria.pdf`) é fundamental e cobre todos os conceitos usados pelo
> código. O enunciado e o `planejamento.md` completam o núcleo. Os demais 11
> PDFs são consulta futura/contexto e **não têm dependência** com este trabalho.
> Não há lacunas relevantes na documentação para a implementação.

---

## Conceitos Fundamentais

| Conceito | Onde no slide-deck | Como aparece no código |
|---|---|---|
| Paginação (página × quadro) | slides 24, 30–31 | `addr >> s`, `Frame`, `PageTableEntry` |
| Tradução lógico→físico / cálculo de `s` | slides 27, 29, 30 | `calcular_bits_offset`, `pagina = addr >> s` |
| Tabela de páginas (present + quadro) | slides 27, 34 | `PageTableEntry{presente, quadro}` |
| Frame table (R, M, last access) | slides 32, 34 | `Frame{R, M, last_access, load_time}` |
| Page fault + substituição | slides 25–26, 46 | `tratar_acesso`, `escolher_vitima` |
| Página suja (M) → escrita ao disco | slide 46 | `stats.paginas_escritas++` quando `M==1` |
| **Ótimo** (futuro mais remoto) | slide 47 | `alg_otimo.c` |
| **NRU** (4 classes, reset de R) | slide 48, 90–92 | `alg_nru.c` (`NRU_RESET_INTERVAL`) |
| **Clock / 2ª chance** | slides 49–51 | `alg_clock.c` (ponteiro circular) |
| **LRU** | slides 52–53 | `alg_lru.c` (menor `last_access`) |
| Localidade / working set / thrashing | slides 56–58 | (contexto p/ justificar resultados no relatório) |
| Anomalia de Belady (FIFO) | slide 62 | (útil p/ análise comparativa) |
| Tamanho de página (trade-offs) | slide 45 | (justifica variação 4KB vs 8KB) |

---

## Plano Inicial de Desenvolvimento

> O código já está implementado. As etapas restantes são de **verificação,
> execução e documentação**, não de implementação do simulador.

### Ordem sugerida

1. **Build & smoke test** *(sem dependências)* — criar `Makefile`, compilar,
   rodar 1 execução de cada algoritmo e conferir que o relatório sai correto.
2. **Validação de corretude** *(depende de 1)* — reproduzir o exemplo manual
   dos slides 90–92 (memória de 4 quadros, sequência dada) e conferir
   page-faults de LRU e NRU; garantir que Ótimo ≤ demais.
3. **Identificação do grupo** *(independente)* — adicionar integrantes nos
   cabeçalhos dos `.c`/`.h` (exigência do enunciado).
4. **Script de execução em lote** *(depende de 1)* — automatizar as 96
   simulações e despejar resultados em CSV/tabela.
5. **Coleta e tabulação dos dados** *(depende de 4)*.
6. **Relatório .pdf** *(depende de 2 e 5)* — resumo, análise de desempenho,
   tabela, pseudo-código do Ótimo, divisão de tarefas do grupo.
7. **Kit de entrega** *(depende de todos)* — `.c`/`.h` + relatório, **sem** os `.log`.

### Dependências entre tarefas

```
1 (build) ──► 2 (validação) ──┐
          └─► 4 (lote) ─► 5 (dados) ──► 6 (relatório) ──► 7 (entrega)
3 (grupo) ────────────────────────────►┘
```

### Riscos / pontos de atenção

- **Relatório é o maior risco de nota** (peso significativo) e ainda não existe.
- **Tempo/memória das 96 execuções**: cada log tem 1M de acessos; o Ótimo aloca
  vetores `proximo_acesso` (1M longs) + `ultima_ocorrencia` (até 2^20 longs).
  Viável, mas monitorar uso de RAM (≈ dezenas de MB por execução).
- **Cabeçalhos sem integrantes** podem custar pontos — corrigir cedo.
- **Escolha de `NRU_RESET_INTERVAL`** deve ser justificada no relatório.
- **Reprodutibilidade**: fixar comando de compilação e versão do compilador.

---

## Observações

- **Não há `Makefile`.** Comando de build manual de referência:
  `gcc -O2 -Wall -o sim-virtual main.c memsim.c alg_lru.c alg_nru.c alg_clock.c alg_otimo.c`
- **Plataforma:** Windows 11; shell primário PowerShell (Bash POSIX também
  disponível). Para gerar `sim-virtual.exe`, MinGW/GCC ou WSL.
- **Logs:** 4 arquivos em `arquivos/` (`compilador`, `compressor`, `matriz`,
  `simulador`), **1.000.000 de linhas cada**. Note que o enunciado cita
  "matrix.log" mas o arquivo presente chama-se **`matriz.log`**.
- **Mapeamento `time` ↔ índice:** o contador `time` do simulador é exatamente o
  índice no vetor de acessos (`main.c:simular`). O Ótimo depende dessa
  coincidência — preservá-la em qualquer refatoração.
- **Design "strategy"** (`algoritmo.h`): para adicionar um algoritmo (ex.: FIFO,
  Aging), basta criar `alg_xxx.c/.h` expondo `obter_algoritmo_xxx()` e
  registrá-lo em `selecionar_algoritmo` (`main.c`). Não é exigido, mas FIFO/2ª
  chance poderiam enriquecer a análise do relatório.
- **Saída esperada** (formato do enunciado):
  ```
  Executando o simulador...
  Arquivo de entrada: arquivo.log
  Tamanho da memoria fisica: 2 MB
  Tamanho das paginas: 8 KB
  Algoritmo de substituicao: LRU
  Numero de Faltas de Paginas: 520
  Numero de Paginas Escritas: 352
  ```
- **Diretriz da sessão atual:** esta sessão foi de **compreensão apenas** —
  nenhuma alteração de código foi feita. Implementações começam nas próximas
  sessões.
