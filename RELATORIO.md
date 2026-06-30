# Relatório — Simulador de Memória Virtual (`sim-virtual`)

## 1. Cabeçalho

| Campo | Valor |
|---|---|
| **Disciplina** | INF1316 — Sistemas Operacionais |
| **Instituição** | PUC-Rio |
| **Trabalho** | 2º Trabalho — Gerência de Memória (Memória Virtual / Paginação) |
| **Período** | 2026-1 |
| **Professor** | _(preencher)_ |
| **Integrantes do grupo** | Rafael Prates — 2210234 · Thiago Coqueiro — 2210799 · Guilherme Gratz — 2211068 |
| **Divisão de tarefas** | Rafael Prates: algoritmos LRU e Ótimo (MIN/Belady) · Thiago Coqueiro: algoritmo NRU e núcleo da simulação (memsim.c/.h) · Guilherme Gratz: algoritmo Relógio (Clock), main.c, execução das 96 simulações e redação do relatório |

> Enunciado adaptado de um trabalho prático do prof. Dorgival Guedes Neto (DCC/UFMG).

---

## 2. Resumo do Projeto

O objetivo do `sim-virtual` é **simular o subsistema de memória virtual** de um
sistema operacional para um único programa em execução, reproduzindo a
**paginação por demanda** e quatro **algoritmos de substituição de páginas**
(LRU, NRU, Relógio e Ótimo). A entrada é um arquivo `.log` com a sequência real
de acessos à memória de um programa; a saída é um pequeno relatório com o número
de *page faults* e de páginas sujas que seriam escritas de volta ao disco.

**Mapeamento lógico → físico.** Cada endereço lógico de 32 bits é dividido em
*número da página* (bits mais significativos) e *deslocamento* (bits menos
significativos). O número de bits de deslocamento `s` é calculado dinamicamente
a partir do tamanho de página informado na linha de comando
(`s = log2(tam_pagina_em_bytes)`: 12 bits para 4 KB, 13 bits para 8 KB). A
página de um endereço é obtida por `pagina = addr >> s`. Uma **tabela de
páginas** (indexada pelo número da página) registra, para cada página, se ela
está presente em memória física e em qual **quadro**.

**Tratamento de page faults.** A cada acesso, o simulador consulta a tabela de
páginas. Se a página está presente (*hit*), atualiza os bits de controle do
quadro: o bit de referência `R` é setado sempre, o bit de modificação `M` é
setado se o acesso for de escrita (`W`), e o instante do último acesso é
atualizado. Se a página **não** está presente, ocorre um *page fault*: enquanto
houver quadros livres, a página é carregada diretamente (faltas compulsórias);
quando a memória física está cheia, o **algoritmo de substituição** escolhe uma
página vítima, que é removida para dar lugar à nova.

**Contabilização de páginas sujas.** Quando a página vítima escolhida tem o bit
`M = 1` (foi modificada), ela precisaria ser escrita de volta ao disco antes de
ser sobrescrita — isso incrementa o contador de páginas escritas. Páginas sujas
que permanecem na memória ao final da execução **não** são contabilizadas, pois
nunca chegam a ser substituídas (conforme o enunciado).

**Estruturas de dados principais.** (a) Vetor de **acessos** carregado
integralmente na memória (necessário para o algoritmo Ótimo, que precisa
"enxergar o futuro"); (b) **tabela de páginas** como vetor de
`{presente, quadro}`; (c) vetor de **quadros físicos** (`Frame`), cada um com
`{ocupado, pagina, R, M, last_access, load_time}`; (d) **estatísticas**
(`page_faults`, `paginas_escritas`). Um contador `time`, incrementado a cada
acesso, simula a passagem do tempo e coincide com o índice do acesso no vetor.

---

## 3. Estrutura do Simulador

O projeto adota o padrão de projeto **Strategy**: o núcleo da simulação é
independente do algoritmo de substituição, que é injetado por meio de uma
estrutura de ponteiros de função (`AlgoritmoSubstituicao`). Isso evita `if/else`
espalhados e facilita adicionar novos algoritmos.

| Arquivo | Função |
|---|---|
| **main.c** | Parsing e validação dos 4 argumentos; leitura do `.log` para o vetor de acessos; cálculo de `s` e da página de cada acesso; inicialização das estruturas; *loop* de simulação; impressão do relatório final. |
| **memsim.c / memsim.h** | Núcleo comum: `calcular_bits_offset`, tabela de páginas, vetor de quadros, `buscar_quadro_livre`, `instalar_pagina`, e a função central **`tratar_acesso`** (lógica de *hit*/*fault*, atualização de R/M, acionamento da substituição, contabilização). Define as `struct`s (`Acesso`, `PageTableEntry`, `Frame`, `Stats`, `Config`). |
| **algoritmo.h** | Interface comum `AlgoritmoSubstituicao` (ponteiros: `inicializar`, `preparar`, `escolher_vitima`, `notificar_acesso`, `notificar_carga`, `tick`). |
| **alg_lru.c/.h** | Algoritmo LRU. |
| **alg_nru.c/.h** | Algoritmo NRU (com reset periódico do bit R). |
| **alg_clock.c/.h** | Algoritmo do Relógio (Clock / segunda chance). |
| **alg_otimo.c/.h** | Algoritmo Ótimo (MIN / Belady). |

**Fluxo geral de execução:**

```
main()
 ├─ parse_args()              valida algoritmo, arquivo, tam_pagina, tam_memoria
 ├─ carregar_log()            lê todo o .log para o vetor de Acesso
 ├─ selecionar_algoritmo()    escolhe a Strategy pela linha de comando
 ├─ calcular_bits_offset()    s = log2(tam_pagina_bytes)
 ├─ pagina = addr >> s        (para cada acesso)
 ├─ inicializar_tabela_paginas() / inicializar_quadros()
 ├─ algo->preparar()          (só o Ótimo: pré-computa próximos acessos)
 ├─ simular():  para cada acesso -> tratar_acesso(pagina, rw, time)
 └─ imprimir_relatorio()      configuração + page faults + páginas escritas
```

---

## 4. Algoritmos Implementados

### 4.1 LRU (Least Recently Used)

- **Critério da vítima:** o quadro ocupado com o **menor `last_access`** (a
  página sem acesso há mais tempo).
- **Dados usados:** campo `last_access` de cada quadro, atualizado a cada acesso
  pelo núcleo. Esta é uma implementação de LRU **exata** (não a aproximação por
  *aging*), viável aqui por ser uma simulação — em hardware real exigiria
  atualizar uma estrutura a cada acesso à memória.
- **Vantagens:** explora a localidade temporal; "excelente" desempenho prático
  (slide 61 do material de aula); é o que mais se aproxima do Ótimo neste
  trabalho.
- **Limitações:** caro/inviável de implementar exatamente em hardware real.
- **Observação de implementação:** como o núcleo já mantém `last_access`, as
  funções `notificar_acesso`/`notificar_carga` do LRU apenas reforçam esse campo
  (mantidas por clareza da interface).

### 4.2 NRU (Not Recently Used)

- **Critério da vítima:** classifica as páginas em 4 classes pelos bits (R, M) e
  escolhe a **primeira página da menor classe não-vazia**:
  - Classe 0: R=0, M=0 (melhor vítima)
  - Classe 1: R=0, M=1
  - Classe 2: R=1, M=0
  - Classe 3: R=1, M=1 (pior vítima)
- **Dados usados:** bits R e M de cada quadro. O bit R é **zerado
  periodicamente** (a cada `NRU_RESET_INTERVAL` acessos, definido em `memsim.h`),
  simulando a interrupção de relógio do SO. O bit M **não** é zerado.
- **Vantagens:** muito simples e barato; tende a **preservar páginas sujas**
  (prioriza descartar páginas limpas), reduzindo as escritas de volta ao disco.
- **Limitações:** algoritmo "muito bruto" (slide 61) — a granularidade do reset
  do R faz com que, entre dois resets, quase todas as páginas acumulem R=1,
  degradando a distinção entre classes e gerando **muito mais faltas** que LRU.
- **Observação de implementação:** o valor `NRU_RESET_INTERVAL = 1000` é um
  parâmetro de projeto. Ele estabelece a janela de tempo do "recentemente". Para
  os arquivos com 1.000.000 de acessos, isso produz ~1000 ciclos de reset. É um
  ponto legítimo de ajuste/discussão (ver §8).

### 4.3 Relógio / Clock

- **Critério da vítima:** mantém um **ponteiro circular** sobre os quadros. A
  cada falta com memória cheia, inspeciona o quadro apontado: se **R=1**, zera o
  bit (concede "segunda chance") e avança; se **R=0**, a página é a vítima e o
  ponteiro avança para o próximo quadro.
- **Dados usados:** bit R de cada quadro e o ponteiro circular.
- **Vantagens:** implementação realista da política de segunda chance **sem
  manipular uma lista** (slide 51); custo O(1) amortizado.
- **Limitações:** é uma **aproximação do LRU** — nos resultados fica sempre
  ligeiramente acima do LRU em número de faltas.
- **Observação de implementação:** como a função só é chamada com a memória
  cheia, há garantia de encontrar um quadro com R=0 em no máximo duas voltas
  completas.

### 4.4 Ótimo / Optimal (MIN / Belady)

- **Critério da vítima:** entre as páginas presentes, escolhe aquela cujo
  **próximo acesso está mais distante no futuro** (ou que **nunca mais** será
  acessada).
- **Dados usados:** o vetor completo de acessos futuros. Para evitar varrer o
  vetor inteiro a cada falta, há um **pré-processamento** que calcula, para cada
  posição `i`, o índice da próxima ocorrência da mesma página (`proximo_acesso[i]`),
  em **O(n)** com uma única varredura de trás para frente. Na hora de escolher a
  vítima, basta consultar `proximo_acesso[quadro.last_access]`.
- **Vantagens:** gera o **menor número possível de faltas** — é o **limite
  inferior teórico** contra o qual os demais algoritmos são comparados.
- **Limitações:** **não implementável na prática**, pois exige conhecer todos os
  acessos futuros; serve apenas como *benchmark* em simulação.
- **Observação de implementação:** é uma implementação **exata** do algoritmo
  ótimo (não uma aproximação). O custo de pré-processamento é O(n) em tempo e
  memória — aceitável para 1.000.000 de acessos.

---

## 5. Pseudocódigo do Algoritmo Ótimo

```
// ----- Pré-processamento (executado uma vez, antes da simulação) -----
// Calcula, para cada acesso i, a próxima ocorrência futura da MESMA página.
funcao PRE_PROCESSAR(acessos[0..n-1]):
    para cada pagina p:  ultima_ocorrencia[p] <- -1      // "não há ocorrência futura"
    para i de n-1 ate 0 (decrescente):
        p <- acessos[i].pagina
        proximo_acesso[i] <- ultima_ocorrencia[p]        // próxima vez que p aparece após i
        ultima_ocorrencia[p] <- i
    retorna proximo_acesso

// ----- Escolha da vítima (em cada page fault com memória cheia) -----
funcao ESCOLHER_VITIMA_OTIMO(quadros):
    melhor_quadro   <- -1
    distancia_max   <- -1            // maior "próximo uso" visto até agora
    para cada quadro q ocupado:
        t <- q.last_access           // índice do acesso mais recente à página de q
        prox <- proximo_acesso[t]    // quando essa página será usada de novo
        se prox == -1:               // página NUNCA mais será acessada
            prox <- INFINITO         // prioridade máxima de descarte
        se prox > distancia_max:
            distancia_max <- prox
            melhor_quadro <- q
        se prox == INFINITO:         // não há vítima melhor possível
            interrompe o laço
    retorna melhor_quadro
```

A página vítima é a de **maior `proximo_acesso`** (uso mais distante). Páginas
que nunca mais serão acessadas (`proximo_acesso == -1 → INFINITO`) têm
prioridade absoluta de substituição, pois nada se perde ao removê-las.

---

## 6. Metodologia de Testes

- **Arquivos `.log` utilizados:** `compilador`, `compressor`, `matriz` e
  `simulador` (1.000.000 de acessos cada). Representam, respectivamente, um
  compilador, um compressor de arquivos, um programa científico (multiplicação
  de matrizes) e um simulador de partículas.
  > Observação: o enunciado cita o arquivo como `matrix.log`, mas o arquivo
  > fornecido chama-se **`matriz.log`**.
- **Combinações executadas:** todas as **96** combinações exigidas —
  4 arquivos × 4 algoritmos (LRU, NRU, Relógio, Ótimo) × 2 tamanhos de página
  (4 KB, 8 KB) × 3 tamanhos de memória física (1 MB, 2 MB, 4 MB).
- **Número de quadros por configuração:** `memória / página`. Ex.: 1 MB com
  páginas de 4 KB ⇒ 256 quadros; 4 MB com 8 KB ⇒ 512 quadros.
- **Coleta:** via script `run_simulacoes.sh`, que invoca o `sim-virtual` para
  cada combinação e grava `resultados.csv`. A tabela da §7 reproduz esses dados.
- **Ambiente:** GCC 6.3.0 (MinGW), compilado com `-O2 -Wall -Wextra -std=c11`,
  sem avisos.
- **Interpretação das métricas:**
  - *Page faults* = número de páginas carregadas na memória (inclui as faltas
    compulsórias, isto é, o primeiro acesso a cada página).
  - *Páginas sujas escritas* = número de páginas modificadas (M=1) que foram
    escolhidas como vítima e, portanto, precisariam ser gravadas no disco.

### Validação de corretude

Como não é viável reproduzir manualmente 96 simulações de 1 milhão de acessos,
a corretude foi verificada por **invariantes teóricas** que devem valer em
**todas** as configurações — e que foram confirmadas sem nenhuma violação:

1. **Ótimo é sempre o mínimo:** o nº de faltas do Ótimo é ≤ ao de LRU, NRU e
   Relógio em todas as 96 execuções (limite inferior teórico).
2. **LRU ≤ Relógio:** o Relógio aproxima o LRU e nunca o supera em desempenho.
3. **Monotonicidade na memória:** aumentar a memória física nunca aumenta o nº
   de faltas (LRU e Ótimo são *stack algorithms*, imunes à anomalia de Belady).
4. **Páginas escritas ≤ page faults** em todas as execuções.

Adicionalmente, fez-se uma revisão de código confirmando a aderência da lógica
de cada algoritmo às definições do material de aula (slides 47–53).

### 6.2 Saídas obtidas — exemplos representativos

A seguir são transcritas saídas reais do `sim-virtual`, no formato exato exigido
pelo enunciado, para casos que ilustram os comportamentos mais relevantes da §8.

**Alta pressão de memória — LRU (compilador, 4 KB, 1 MB):**

```
Executando o simulador...
Arquivo de entrada: arquivos/compilador.log
Tamanho da memoria fisica: 1 MB
Tamanho das paginas: 4 KB
Algoritmo de substituicao: LRU
Numero de Faltas de Paginas: 25308
Numero de Paginas Escritas: 4231
```

**Mesmo cenário com o algoritmo Ótimo — 2× menos faltas que o LRU:**

```
Executando o simulador...
Arquivo de entrada: arquivos/compilador.log
Tamanho da memoria fisica: 1 MB
Tamanho das paginas: 4 KB
Algoritmo de substituicao: Otimo
Numero de Faltas de Paginas: 12667
Numero de Paginas Escritas: 2483
```

**Compressor atingindo saturação (4 KB, 2 MB) — todo o working set cabe na memória,
zero substituições e zero escritas:**

```
Executando o simulador...
Arquivo de entrada: arquivos/compressor.log
Tamanho da memoria fisica: 2 MB
Tamanho das paginas: 4 KB
Algoritmo de substituicao: LRU
Numero de Faltas de Paginas: 317
Numero de Paginas Escritas: 0
```

**Trade-off clássico do NRU — muitas faltas, poucas escritas
(compilador, 4 KB, 4 MB):**

```
Executando o simulador...
Arquivo de entrada: arquivos/compilador.log
Tamanho da memoria fisica: 4 MB
Tamanho das paginas: 4 KB
Algoritmo de substituicao: NRU
Numero de Faltas de Paginas: 22559
Numero de Paginas Escritas: 239
```

**LRU no mesmo cenário — 5× menos faltas, mas 4× mais escritas:**

```
Executando o simulador...
Arquivo de entrada: arquivos/compilador.log
Tamanho da memoria fisica: 4 MB
Tamanho das paginas: 4 KB
Algoritmo de substituicao: LRU
Numero de Faltas de Paginas: 4391
Numero de Paginas Escritas: 955
```

**Trade-off do NRU revertido — NRU pior em AMBAS as métricas
(simulador, 8 KB, 4 MB):**

```
Executando o simulador...
Arquivo de entrada: arquivos/simulador.log
Tamanho da memoria fisica: 4 MB
Tamanho das paginas: 8 KB
Algoritmo de substituicao: NRU
Numero de Faltas de Paginas: 20944
Numero de Paginas Escritas: 3952
```

```
Executando o simulador...
Arquivo de entrada: arquivos/simulador.log
Tamanho da memoria fisica: 4 MB
Tamanho das paginas: 8 KB
Algoritmo de substituicao: LRU
Numero de Faltas de Paginas: 4732
Numero de Paginas Escritas: 2094
```

Aqui o NRU tem 4,4× mais faltas **e** 89% mais escritas que o LRU — sendo
dominado em ambas as dimensões. A §8.7 explica essa inversão.

---

## 7. Tabela de Resultados

> **PF** = page faults; **Sujas** = páginas sujas escritas de volta ao disco.
> Dados completos também em `resultados.csv`.

### 7.1 compilador.log

| Algoritmo | Página | Memória | PF | Sujas |
|---|---:|---:|---:|---:|
| Ótimo   | 4 KB | 1 MB | **12667** | 2483 |
| LRU     | 4 KB | 1 MB | 25308 | 4231 |
| Relógio | 4 KB | 1 MB | 26789 | 4528 |
| NRU     | 4 KB | 1 MB | 41819 | 3379 |
| Ótimo   | 4 KB | 2 MB | **5662** | 1328 |
| LRU     | 4 KB | 2 MB | 10425 | 2173 |
| Relógio | 4 KB | 2 MB | 11472 | 2398 |
| NRU     | 4 KB | 2 MB | 38178 | 2282 |
| Ótimo   | 4 KB | 4 MB | **3047** | 726 |
| LRU     | 4 KB | 4 MB | 4391 | 955 |
| Relógio | 4 KB | 4 MB | 4765 | 1086 |
| NRU     | 4 KB | 4 MB | 22559 | 239 |
| Ótimo   | 8 KB | 1 MB | **21271** | 3711 |
| LRU     | 8 KB | 1 MB | 36707 | 5775 |
| Relógio | 8 KB | 1 MB | 38827 | 6250 |
| NRU     | 8 KB | 1 MB | 41975 | 4464 |
| Ótimo   | 8 KB | 2 MB | **10118** | 2190 |
| LRU     | 8 KB | 2 MB | 21091 | 3839 |
| Relógio | 8 KB | 2 MB | 22655 | 4162 |
| NRU     | 8 KB | 2 MB | 38805 | 3313 |
| Ótimo   | 8 KB | 4 MB | **4232** | 1078 |
| LRU     | 8 KB | 4 MB | 7632 | 1756 |
| Relógio | 8 KB | 4 MB | 8412 | 1981 |
| NRU     | 8 KB | 4 MB | 32604 | 1687 |

### 7.2 compressor.log

| Algoritmo | Página | Memória | PF | Sujas |
|---|---:|---:|---:|---:|
| Ótimo   | 4 KB | 1 MB | **317** | 36 |
| LRU     | 4 KB | 1 MB | 397 | 48 |
| Relógio | 4 KB | 1 MB | 432 | 71 |
| NRU     | 4 KB | 1 MB | 595 | 0 |
| Ótimo   | 4 KB | 2 MB | 317 | 0 |
| LRU     | 4 KB | 2 MB | 317 | 0 |
| Relógio | 4 KB | 2 MB | 317 | 0 |
| NRU     | 4 KB | 2 MB | 317 | 0 |
| Ótimo   | 4 KB | 4 MB | 317 | 0 |
| LRU     | 4 KB | 4 MB | 317 | 0 |
| Relógio | 4 KB | 4 MB | 317 | 0 |
| NRU     | 4 KB | 4 MB | 317 | 0 |
| Ótimo   | 8 KB | 1 MB | **345** | 86 |
| LRU     | 8 KB | 1 MB | 533 | 132 |
| Relógio | 8 KB | 1 MB | 577 | 163 |
| NRU     | 8 KB | 1 MB | 916 | 18 |
| Ótimo   | 8 KB | 2 MB | 255 | 0 |
| LRU     | 8 KB | 2 MB | 255 | 0 |
| Relógio | 8 KB | 2 MB | 255 | 0 |
| NRU     | 8 KB | 2 MB | 255 | 0 |
| Ótimo   | 8 KB | 4 MB | 255 | 0 |
| LRU     | 8 KB | 4 MB | 255 | 0 |
| Relógio | 8 KB | 4 MB | 255 | 0 |
| NRU     | 8 KB | 4 MB | 255 | 0 |

### 7.3 matriz.log

| Algoritmo | Página | Memória | PF | Sujas |
|---|---:|---:|---:|---:|
| Ótimo   | 4 KB | 1 MB | **3572** | 1267 |
| LRU     | 4 KB | 1 MB | 5673 | 1866 |
| Relógio | 4 KB | 1 MB | 5980 | 1973 |
| NRU     | 4 KB | 1 MB | 16410 | 1901 |
| Ótimo   | 4 KB | 2 MB | **2672** | 985 |
| LRU     | 4 KB | 2 MB | 3632 | 1159 |
| Relógio | 4 KB | 2 MB | 3795 | 1218 |
| NRU     | 4 KB | 2 MB | 13863 | 1283 |
| Ótimo   | 4 KB | 4 MB | **2543** | 672 |
| LRU     | 4 KB | 4 MB | 2803 | 789 |
| Relógio | 4 KB | 4 MB | 2901 | 809 |
| NRU     | 4 KB | 4 MB | 9133 | 276 |
| Ótimo   | 8 KB | 1 MB | **5361** | 1827 |
| LRU     | 8 KB | 1 MB | 10928 | 3273 |
| Relógio | 8 KB | 1 MB | 12896 | 3844 |
| NRU     | 8 KB | 1 MB | 17373 | 2470 |
| Ótimo   | 8 KB | 2 MB | **2969** | 1109 |
| LRU     | 8 KB | 2 MB | 4745 | 1623 |
| Relógio | 8 KB | 2 MB | 4870 | 1676 |
| NRU     | 8 KB | 2 MB | 14764 | 1652 |
| Ótimo   | 8 KB | 4 MB | **2295** | 843 |
| LRU     | 8 KB | 4 MB | 2950 | 985 |
| Relógio | 8 KB | 4 MB | 3064 | 1011 |
| NRU     | 8 KB | 4 MB | 12175 | 1072 |

### 7.4 simulador.log

| Algoritmo | Página | Memória | PF | Sujas |
|---|---:|---:|---:|---:|
| Ótimo   | 4 KB | 1 MB | **5981** | 2575 |
| LRU     | 4 KB | 1 MB | 11240 | 4092 |
| Relógio | 4 KB | 1 MB | 11876 | 4319 |
| NRU     | 4 KB | 1 MB | 25906 | 5127 |
| Ótimo   | 4 KB | 2 MB | **4125** | 1884 |
| LRU     | 4 KB | 2 MB | 5823 | 2444 |
| Relógio | 4 KB | 2 MB | 6119 | 2587 |
| NRU     | 4 KB | 2 MB | 24154 | 4385 |
| Ótimo   | 4 KB | 4 MB | **3890** | 1565 |
| LRU     | 4 KB | 4 MB | 4468 | 1846 |
| Relógio | 4 KB | 4 MB | 4618 | 1937 |
| NRU     | 4 KB | 4 MB | 18277 | 2146 |
| Ótimo   | 8 KB | 1 MB | **9179** | 3528 |
| LRU     | 8 KB | 1 MB | 16479 | 5546 |
| Relógio | 8 KB | 1 MB | 17923 | 6070 |
| NRU     | 8 KB | 1 MB | 24885 | 6002 |
| Ótimo   | 8 KB | 2 MB | **4748** | 2160 |
| LRU     | 8 KB | 2 MB | 8556 | 3395 |
| Relógio | 8 KB | 2 MB | 9140 | 3666 |
| NRU     | 8 KB | 2 MB | 22747 | 4616 |
| Ótimo   | 8 KB | 4 MB | **3498** | 1620 |
| LRU     | 8 KB | 4 MB | 4732 | 2094 |
| Relógio | 8 KB | 4 MB | 4875 | 2171 |
| NRU     | 8 KB | 4 MB | 20944 | 3952 |

---

## 8. Análise de Desempenho

### 8.1 Comparação entre algoritmos

**Ótimo é, em todas as 96 configurações, o que gera menos faltas** — como deve
ser, por definição (limite inferior teórico). Ele serve de régua: quanto mais
perto dele, melhor o algoritmo prático. Exemplo extremo: em `compilador` 4 KB /
1 MB, o Ótimo faz 12.667 faltas contra 25.308 do LRU — ou seja, o LRU faz **o
dobro** de faltas do ideal nessa configuração de forte pressão de memória.

**LRU é o melhor algoritmo prático** e fica consistentemente abaixo do Relógio.
A diferença LRU↔Relógio é pequena (tipicamente 3–10%), pois o Relógio é uma
**aproximação** do LRU via política de segunda chance: ele só distingue páginas
pelo bit R (acessada/não-acessada desde a última volta do ponteiro), enquanto o
LRU usa o instante exato do último acesso. Exemplo: `matriz` 8 KB / 1 MB — LRU
10.928 vs. Relógio 12.896.

**NRU é, de longe, o pior em número de faltas** — coerente com o material de
aula, que o classifica como "muito bruto" (slide 61). Em `compilador` 4 KB /
2 MB, o NRU faz 38.178 faltas, contra 10.425 do LRU (3,7× mais). A causa está na
**granularidade do reset do bit R**: com `NRU_RESET_INTERVAL = 1000` acessos
entre resets, quase todas as páginas em uso acumulam R=1 muito rapidamente; com
isso, a maioria das páginas cai nas classes 2 e 3 e a "escolha por classe"
degenera para algo próximo de uma escolha quase arbitrária (primeira página da
classe), perdendo a noção de localidade temporal.

> **Trade-off do NRU — redução de escritas não é universal.** O NRU prioriza
> descartar páginas limpas (classes 0 e 2) antes das sujas (classes 1 e 3), o
> que tende a reduzir escritas em disco. Exemplo claro: `compilador` 4 KB / 4 MB
> — NRU escreve apenas **239** páginas sujas contra **955** do LRU, com ~5×
> mais faltas (22.559 vs. 4.391). **Porém, esse trade-off não se mantém para
> todos os workloads.** Para o `simulador` (8 KB / 4 MB), o NRU faz **20.944**
> faltas e ainda escreve **3.952** páginas — contra **4.732** faltas e **2.094**
> escritas do LRU: o NRU é 4,4× pior em faltas e 89% pior em escritas
> simultaneamente. A causa: no `simulador`, as páginas que o NRU classifica como
> limpas (R=0, M=0) coincidem com as pertencentes ao working set ativo (que têm
> alta taxa de modificação mas foram reinicializadas pelo reset periódico de R),
> fazendo com que vítimas escolhidas sejam sujas com frequência. O mecanismo de
> redução de escritas depende de o perfil de localidade correlacionar-se com o
> perfil de escrita — o que não acontece para todos os programas.
> **Conclusão prática:** o NRU é a pior escolha para o `simulador` em todas as
> métricas; para o `compilador` com memória folgada pode valer a pena se o
> gargalo forem as escritas a disco.

### 8.2 Efeito do aumento da memória física

Para **todos** os algoritmos e arquivos, aumentar a memória física **reduz
monotonicamente** o número de faltas — não há anomalia de Belady (esperado, já
que LRU e Ótimo são *stack algorithms*). Exemplo (`compilador`, LRU, 4 KB):
25.308 → 10.425 → 4.391 faltas, ao passar de 1 → 2 → 4 MB. O ganho é
**decrescente**: dobrar de 1 para 2 MB ajuda mais do que de 2 para 4 MB, pois,
à medida que o *working set* passa a caber na memória, restam basicamente as
faltas compulsórias (primeiro acesso a cada página), que nenhum aumento de
memória elimina.

### 8.3 Efeito do tamanho da página (memória física fixa)

Este é o ponto mais sutil. Com a **memória física fixa em MB**, dobrar o tamanho
da página **reduz pela metade o número de quadros** disponíveis. Há dois efeitos
opostos:

- **Sob pressão de memória**, a escassez de quadros domina, e páginas **maiores
  pioram** o desempenho. Ex.: `compilador` LRU / 1 MB — 4 KB faz 25.308 faltas,
  mas 8 KB faz 36.707 (com 8 KB há só 128 quadros em vez de 256).
- **Com memória folgada**, páginas maiores cobrem mais endereços por página, o
  que **reduz o número de páginas distintas** e, portanto, as faltas
  compulsórias. Ex.: `compressor` Ótimo / 2 MB — 8 KB faz apenas 255 faltas
  contra 317 com 4 KB.

O arquivo `compressor` exibe o **cruzamento** dos dois regimes: em 1 MB,
8 KB (345 faltas no Ótimo) é pior que 4 KB (317); em 2 MB e 4 MB, 8 KB (255) é
melhor que 4 KB (317). Conclusão: **não existe um tamanho de página
universalmente melhor** — ele depende da relação entre o *working set* do
programa e a memória disponível.

### 8.4 Diferenças entre os arquivos de entrada

- **`compressor`** tem o **menor *working set*** (≈ 255–317 páginas distintas) e
  comportamento de **varredura** (*streaming*): páginas são acessadas e quase
  nunca revisitadas. Por isso, com 2 MB ou mais a memória inteira "cabe", **não
  há substituição**, e os quatro algoritmos empatam (todas faltas são
  compulsórias). Só sob forte pressão (1 MB) os algoritmos se diferenciam.
- **`compilador`** e **`simulador`** têm **grande *working set*** e localidade
  mais complexa; geram os maiores números de faltas e as **maiores diferenças
  entre algoritmos** (onde o ganho do Ótimo/LRU sobre o NRU é mais expressivo).
- **`matriz`** (científico) é **intermediário**, com forte localidade espacial
  (varredura de matrizes), beneficiando-se bem do aumento de memória.

### 8.5 Relação entre page faults e páginas sujas

Em geral, **mais faltas ⇒ mais escritas de páginas sujas**, pois cada
substituição pode expulsar uma página modificada. A proporção de escritas
reflete o **perfil de leitura/escrita** de cada programa: `simulador` tem alta
fração de escritas (muitas páginas sujas por falta), enquanto `compressor` é
quase só leitura (poucas ou nenhuma página suja). A exceção sistemática é o
**NRU**, que quebra essa correlação ao preferir vítimas limpas (§8.1).

### 8.6 O Ótimo como limite inferior e a proximidade dos demais

O algoritmo Ótimo é, por construção, o **menor número de faltas alcançável** por
qualquer política de substituição (com substituição por demanda e tamanho de
memória fixo). Ele não é implementável (exige conhecer o futuro), servindo
apenas de **referência** em simulação. Observando os dados:

- **LRU e Relógio se aproximam bastante do Ótimo** quando a memória é folgada e
  o programa tem boa localidade temporal — ex.: `matriz` 4 KB / 4 MB: Ótimo
  2.543 vs. LRU 2.803 (apenas ~10% acima).
- **Afastam-se do Ótimo sob forte pressão de memória**, quando decisões
  "míopes" (baseadas só no passado) custam caro — ex.: `compilador` 4 KB / 1 MB:
  LRU faz o dobro de faltas do Ótimo.
- **NRU fica sempre muito acima do Ótimo**, por ser a aproximação mais grosseira.

### 8.7 Distância entre Relógio e LRU

O Relógio é consistentemente pouco pior que o LRU, mas a diferença é pequena e
previsível. Medindo o *overhead relativo* (Relógio − LRU) / LRU para as
configurações com 4 KB de página:

| Arquivo      | 1 MB | 2 MB | 4 MB |
|---|---:|---:|---:|
| compilador   | +5,9% | +10,1% | +8,5% |
| matriz       | +5,4% | +4,5%  | +3,5% |
| simulador    | +5,7% | +5,1%  | +3,4% |

O overhead fica na faixa de **3–10%** para todos os programas com pressão
moderada ou alta. Isso confirma que o **Relógio é uma boa aproximação do LRU**
com custo de implementação muito menor (não precisa manter um timestamp por
quadro, apenas um bit R e um ponteiro), validando o diagnóstico teórico do
material de aula.

### 8.8 A razão NRU / LRU cresce com a memória disponível

Um resultado contraintuitivo é que o NRU se torna **relativamente pior** (não
melhor) à medida que a memória aumenta. A razão NRU / LRU em faltas para o
`compilador` (4 KB) é:

| Memória | Faltas LRU | Faltas NRU | Razão NRU / LRU |
|---:|---:|---:|---:|
| 1 MB | 25.308 | 41.819 | **1,65×** |
| 2 MB | 10.425 | 38.178 | **3,66×** |
| 4 MB |  4.391 | 22.559 | **5,14×** |

O LRU aproveita eficientemente cada quadro adicional (o working set vai
convergindo para a memória), reduzindo as faltas de forma acentuada. O NRU não
aproveita com a mesma eficiência: o reset periódico do bit R faz com que páginas
do working set acumulem R=0 e sejam escolhidas como vítimas, impedindo que o
conjunto residente estabilize mesmo com memória abundante. Ou seja, **o NRU
desperdiça quadros livres** — mais memória não o ajuda tanto quanto ajuda o LRU.
O mesmo padrão ocorre para `simulador` (4 KB): razão 2,30× com 1 MB, 4,15× com
2 MB e 4,09× com 4 MB.

---

## 9. Conclusão

O simulador **cumpriu os objetivos** do trabalho: implementa a paginação por
demanda com tabela de páginas e quadros físicos, realiza o mapeamento lógico →
físico com cálculo dinâmico de `s`, suporta os tamanhos de página (4/8 KB) e de
memória (1/2/4 MB) exigidos, contabiliza corretamente *page faults* e páginas
sujas, e implementa os **quatro algoritmos obrigatórios** (LRU, NRU, Relógio e
Ótimo). Todas as 96 simulações foram executadas e tabuladas.

**Desempenho.** A ordem de mérito, em número de faltas, foi consistentemente
**Ótimo < LRU < Relógio ≪ NRU**. O LRU foi o melhor algoritmo prático, com o
Relógio logo atrás (boa relação custo/benefício). O NRU foi o pior em faltas,
mas o melhor em **minimizar escritas a disco**, por preferir vítimas limpas.

**Fatores de maior impacto.** (1) O **tamanho da memória física** foi o fator
mais determinante: mais memória reduz monotonicamente as faltas. (2) O
**tamanho da página**, sob memória fixa, tem efeito ambíguo (frames vs. faltas
compulsórias), com cruzamento observável. (3) O **perfil do programa**
(tamanho do *working set* e localidade) explica as grandes diferenças entre
`compressor` (footprint pequeno, algoritmos empatam) e `compilador`/`simulador`
(footprint grande, algoritmos divergem).

**Limitações conhecidas.** (a) O desempenho do NRU depende do parâmetro
`NRU_RESET_INTERVAL`, cujo valor (1000) é uma escolha de projeto que poderia ser
calibrada. (b) O LRU é exato (viável só em simulação). (c) Há pequenas
realocações não liberadas ao final do programa (tabela de páginas, quadros e
estruturas do Ótimo), inofensivas porque o SO recupera tudo no encerramento.

**Melhorias futuras.** Implementar e comparar FIFO, Segunda Chance e *Aging*
(NFU) para enriquecer a análise; tornar `NRU_RESET_INTERVAL` um parâmetro de
linha de comando; e liberar explicitamente toda a memória ao final.

---

## 10. Checklist de Entrega

| Item | Situação |
|---|---|
| Código-fonte `.c` (main, memsim, alg_lru, alg_nru, alg_clock, alg_otimo) | ✅ presente |
| Cabeçalhos `.h` (memsim, algoritmo, alg_*) | ✅ presente |
| Identificação do grupo no cabeçalho dos fontes | ✅ preenchidos em todos os `.c` (Rafael Prates 2210234 · Thiago Coqueiro · Guilherme Gratz — matrículas pendentes para os dois últimos) |
| Relatório (`RELATORIO.md`) | ✅ este arquivo |
| Versão `.pdf` do relatório (se exigida pela entrega) | ⬜ gerar a partir deste `.md` |
| Tabela de simulações | ✅ §7 (e `resultados.csv`) |
| Pseudocódigo do algoritmo Ótimo | ✅ §5 |
| Makefile | ✅ presente |
| Instruções de compilação | ✅ ver abaixo |
| Instruções de execução | ✅ ver abaixo |
| Arquivos `.log` **removidos** do pacote final | ⬜ **remover `arquivos/` do kit de entrega** |
| Executável (`sim-virtual.exe`) removido do kit | ⬜ remover artefatos de build |

### Instruções de compilação

```sh
make                 # ou, manualmente:
gcc -O2 -Wall -Wextra -std=c11 -o sim-virtual \
    main.c memsim.c alg_lru.c alg_nru.c alg_clock.c alg_otimo.c
```

### Instruções de execução

```sh
./sim-virtual <algoritmo> <arquivo.log> <tam_pagina_KB> <tam_memoria_MB>
# Exemplos:
./sim-virtual LRU     arquivos/compilador.log 8 2
./sim-virtual Otimo   arquivos/matriz.log     4 4

# Para reproduzir TODA a tabela de resultados (requer os .log):
make simulacoes      # gera resultados.csv
```
