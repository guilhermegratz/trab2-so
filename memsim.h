#ifndef MEMSIM_H
#define MEMSIM_H

/*
 * memsim.h
 *
 * Definições de structs e constantes usadas pelo simulador de
 * memória virtual (sim-virtual), além dos protótipos das funções
 * comuns implementadas em memsim.c.
 */

/* Período (em número de acessos processados) usado pelo NRU para
 * resetar periodicamente o bit R de todos os quadros ocupados.
 * Facilmente ajustável aqui. */
#define NRU_RESET_INTERVAL 1000

/* Representa um acesso lido do arquivo de log: endereço lógico,
 * tipo de acesso (R/W) e, após o cálculo de s, a página correspondente. */
typedef struct {
    unsigned int addr;
    char rw;              /* 'R' ou 'W' */
    unsigned int pagina;  /* preenchido após o cálculo de s */
} Acesso;

/* Entrada da tabela de páginas: indica se a página está presente em
 * memória física e, se sim, em qual quadro. */
typedef struct {
    int presente;
    int quadro;
} PageTableEntry;

/* Representa um quadro de página física. Os campos R, M e
 * last_access são usados por (quase) todos os algoritmos; load_time
 * fica disponível para eventuais critérios de desempate. */
typedef struct {
    int ocupado;
    unsigned int pagina;
    int R;
    int M;
    long last_access; /* instante (time) do último acesso (ou carga) */
    long load_time;   /* instante (time) em que a página foi carregada */
} Frame;

/* Estatísticas coletadas durante a simulação. */
typedef struct {
    long page_faults;
    long paginas_escritas;
} Stats;

/* Configuração obtida a partir da linha de comando. */
typedef struct {
    char algoritmo[32];
    char arquivo[256];
    int tam_pagina_kb;
    int tam_memoria_mb;
} Config;

/* Declaração antecipada (a definição completa está em algoritmo.h),
 * para que memsim.h não precise incluir algoritmo.h. */
struct AlgoritmoSubstituicao;

/* Índice (no vetor de acessos) do acesso sendo processado no momento;
 * coincide com o valor de "time" passado a tratar_acesso(). Mantido
 * aqui para eventual uso pelos algoritmos (ex.: o Ótimo). */
extern long g_indice_acesso_atual;

/* ---- Tradução de endereço lógico -> página ---- */
int calcular_bits_offset(int tamanho_pagina_kb);

/* ---- Estruturas de dados ---- */
void inicializar_tabela_paginas(int num_paginas_max);
void inicializar_quadros(int num_quadros);

int buscar_quadro_livre(void);
void instalar_pagina(unsigned int pagina, int quadro, long time);

/* Função central chamada pelo loop de simulação para cada acesso.
 * Retorna 1 se houve page fault, 0 caso contrário. */
int tratar_acesso(unsigned int pagina, char rw, long time);

/* Acesso ao estado interno (usado pelos módulos de algoritmo). */
Frame *obter_quadros(void);
int obter_num_quadros(void);
PageTableEntry *obter_tabela_paginas(void);
Stats *obter_stats(void);

/* Define qual algoritmo de substituição será usado por tratar_acesso(). */
void memsim_set_algoritmo(struct AlgoritmoSubstituicao *algo);

#endif /* MEMSIM_H */
