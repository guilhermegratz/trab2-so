/*
 * memsim.c
 *
 * Funções de tradução de endereço lógico -> página, controle da
 * tabela de páginas e dos quadros de página física, contabilização
 * de estatísticas, e a lógica central de tratamento de um acesso
 * à memória (tratar_acesso), comum a todos os algoritmos.
 *
 * Integrantes do grupo:
 *   Rafael Prates       - 2210234
 *   Thiago Coqueiro     - (matrícula pendente)
 *   Guilherme Gratz     - (matrícula pendente)
 */

#include <stdio.h>
#include <stdlib.h>
#include "memsim.h"
#include "algoritmo.h"

static PageTableEntry *tabela_paginas = NULL;
static int num_paginas_max = 0;

static Frame *quadros = NULL;
static int num_quadros = 0;

static Stats stats = {0, 0};

static AlgoritmoSubstituicao *algoritmo_atual = NULL;

long g_indice_acesso_atual = 0;

/* Calcula quantos bits de deslocamento (s) usar a partir do tamanho
 * de página em KB, suportando qualquer tamanho que seja potência de
 * 2 (em particular, 4KB e 8KB, exigidos pelo enunciado). */
int calcular_bits_offset(int tamanho_pagina_kb) {
    long bytes = (long)tamanho_pagina_kb * 1024;
    int s = 0;
    while ((1L << s) < bytes) {
        s++;
    }
    return s;
}

void inicializar_tabela_paginas(int n) {
    num_paginas_max = n;
    tabela_paginas = calloc((size_t)num_paginas_max, sizeof(PageTableEntry));
    if (tabela_paginas == NULL) {
        fprintf(stderr, "Erro ao alocar tabela de paginas\n");
        exit(1);
    }
    for (int i = 0; i < num_paginas_max; i++) {
        tabela_paginas[i].presente = 0;
        tabela_paginas[i].quadro = -1;
    }
}

void inicializar_quadros(int n) {
    num_quadros = n;
    quadros = calloc((size_t)num_quadros, sizeof(Frame));
    if (quadros == NULL) {
        fprintf(stderr, "Erro ao alocar quadros de pagina\n");
        exit(1);
    }
    for (int i = 0; i < num_quadros; i++) {
        quadros[i].ocupado = 0;
        quadros[i].pagina = 0;
        quadros[i].R = 0;
        quadros[i].M = 0;
        quadros[i].last_access = -1;
        quadros[i].load_time = -1;
    }
}

int buscar_quadro_livre(void) {
    for (int i = 0; i < num_quadros; i++) {
        if (!quadros[i].ocupado) {
            return i;
        }
    }
    return -1;
}

void instalar_pagina(unsigned int pagina, int quadro, long time) {
    quadros[quadro].ocupado = 1;
    quadros[quadro].pagina = pagina;
    quadros[quadro].R = 1;
    quadros[quadro].M = 0;
    quadros[quadro].last_access = time;
    quadros[quadro].load_time = time;

    tabela_paginas[pagina].presente = 1;
    tabela_paginas[pagina].quadro = quadro;

    if (algoritmo_atual != NULL && algoritmo_atual->notificar_carga != NULL) {
        algoritmo_atual->notificar_carga(quadro, time);
    }
}

int tratar_acesso(unsigned int pagina, char rw, long time) {
    g_indice_acesso_atual = time;

    if (algoritmo_atual != NULL && algoritmo_atual->tick != NULL) {
        algoritmo_atual->tick(time);
    }

    if (tabela_paginas[pagina].presente) {
        int quadro = tabela_paginas[pagina].quadro;
        quadros[quadro].R = 1;
        if (rw == 'W' || rw == 'w') {
            quadros[quadro].M = 1;
        }
        quadros[quadro].last_access = time;

        if (algoritmo_atual != NULL && algoritmo_atual->notificar_acesso != NULL) {
            algoritmo_atual->notificar_acesso(quadro, time);
        }
        return 0;
    }

    /* Page fault */
    stats.page_faults++;

    int quadro_livre = buscar_quadro_livre();
    if (quadro_livre != -1) {
        /* Ainda há quadros livres: instala diretamente, sem acionar
         * o algoritmo de substituição. */
        instalar_pagina(pagina, quadro_livre, time);
        if (rw == 'W' || rw == 'w') {
            quadros[quadro_livre].M = 1;
        }
        return 1;
    }

    /* Memória cheia: aciona o algoritmo de substituição configurado. */
    int vitima = algoritmo_atual->escolher_vitima();
    unsigned int pagina_vitima = quadros[vitima].pagina;

    if (quadros[vitima].M) {
        /* Página suja: seria escrita de volta no disco. */
        stats.paginas_escritas++;
    }

    tabela_paginas[pagina_vitima].presente = 0;
    tabela_paginas[pagina_vitima].quadro = -1;

    instalar_pagina(pagina, vitima, time);
    if (rw == 'W' || rw == 'w') {
        quadros[vitima].M = 1;
    }

    return 1;
}

Frame *obter_quadros(void) {
    return quadros;
}

int obter_num_quadros(void) {
    return num_quadros;
}

PageTableEntry *obter_tabela_paginas(void) {
    return tabela_paginas;
}

Stats *obter_stats(void) {
    return &stats;
}

void memsim_set_algoritmo(struct AlgoritmoSubstituicao *algo) {
    algoritmo_atual = (AlgoritmoSubstituicao *)algo;
}
