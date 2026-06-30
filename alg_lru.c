/*
 * alg_lru.c
 *
 * Algoritmo LRU (Least Recently Used): escolhe como vítima o quadro
 * ocupado com o menor timestamp de último acesso (campo
 * Frame.last_access, mantido por memsim.c a cada acesso/carga).
 *
 * Integrantes do grupo:
 *   Rafael Prates       - 2210234
 *   Thiago Coqueiro     - 2210799
 *   Guilherme Gratz     - 2211068
 */

#include <stddef.h>
#include "alg_lru.h"
#include "memsim.h"

/* Percorre os quadros ocupados e retorna o de menor last_access. */
static int lru_escolher_vitima(void) {
    Frame *quadros = obter_quadros();
    int num_quadros = obter_num_quadros();

    int melhor = -1;
    long menor_time = 0;

    for (int i = 0; i < num_quadros; i++) {
        if (!quadros[i].ocupado) {
            continue;
        }
        if (melhor == -1 || quadros[i].last_access < menor_time) {
            melhor = i;
            menor_time = quadros[i].last_access;
        }
    }
    return melhor;
}

/* Atualiza o timestamp do quadro acessado (last_access já é
 * atualizado por memsim.c, mas mantemos esta função para seguir a
 * interface comum e deixar explícito o que o LRU precisa saber). */
static void lru_notificar_acesso(int quadro, long time) {
    obter_quadros()[quadro].last_access = time;
}

static void lru_notificar_carga(int quadro, long time) {
    obter_quadros()[quadro].last_access = time;
}

static AlgoritmoSubstituicao algoritmo_lru = {
    .inicializar = NULL,
    .preparar = NULL,
    .escolher_vitima = lru_escolher_vitima,
    .notificar_acesso = lru_notificar_acesso,
    .notificar_carga = lru_notificar_carga,
    .tick = NULL
};

AlgoritmoSubstituicao *obter_algoritmo_lru(void) {
    return &algoritmo_lru;
}
