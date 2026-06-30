/*
 * alg_nru.c
 *
 * Algoritmo NRU (Not Recently Used): classifica as páginas em 4
 * categorias, de acordo com os bits R e M, e escolhe como vítima a
 * primeira página encontrada na categoria mais baixa não vazia:
 *
 *   classe 0: R=0, M=0  (melhor vítima)
 *   classe 1: R=0, M=1
 *   classe 2: R=1, M=0
 *   classe 3: R=1, M=1  (pior vítima)
 *
 * O bit R de todos os quadros ocupados é periodicamente zerado, a
 * cada NRU_RESET_INTERVAL acessos processados (definido em
 * memsim.h), simulando o reset feito por um timer/relógio do SO.
 *
 * Integrantes do grupo:
 *   Rafael Prates       - 2210234
 *   Thiago Coqueiro     - 2210799
 *   Guilherme Gratz     - 2211068
 */

#include <stddef.h>
#include "alg_nru.h"
#include "memsim.h"

static long ultimo_reset = 0;

static int nru_escolher_vitima(void) {
    Frame *quadros = obter_quadros();
    int num_quadros = obter_num_quadros();

    int melhor_por_classe[4] = {-1, -1, -1, -1};

    for (int i = 0; i < num_quadros; i++) {
        if (!quadros[i].ocupado) {
            continue;
        }
        int classe = (quadros[i].R ? 2 : 0) + (quadros[i].M ? 1 : 0);
        if (melhor_por_classe[classe] == -1) {
            melhor_por_classe[classe] = i;
        }
    }

    for (int classe = 0; classe < 4; classe++) {
        if (melhor_por_classe[classe] != -1) {
            return melhor_por_classe[classe];
        }
    }
    return -1;
}

/* A cada NRU_RESET_INTERVAL acessos processados, zera o bit R de
 * todos os quadros ocupados. */
static void nru_tick(long time) {
    if (time - ultimo_reset >= NRU_RESET_INTERVAL) {
        Frame *quadros = obter_quadros();
        int num_quadros = obter_num_quadros();
        for (int i = 0; i < num_quadros; i++) {
            if (quadros[i].ocupado) {
                quadros[i].R = 0;
            }
        }
        ultimo_reset = time;
    }
}

static void nru_inicializar(int num_quadros) {
    (void)num_quadros;
    ultimo_reset = 0;
}

static AlgoritmoSubstituicao algoritmo_nru = {
    .inicializar = nru_inicializar,
    .preparar = NULL,
    .escolher_vitima = nru_escolher_vitima,
    .notificar_acesso = NULL,
    .notificar_carga = NULL,
    .tick = nru_tick
};

AlgoritmoSubstituicao *obter_algoritmo_nru(void) {
    return &algoritmo_nru;
}
