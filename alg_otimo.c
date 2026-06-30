/*
 * alg_otimo.c
 *
 * Algoritmo Ótimo (MIN/Belady): entre as páginas presentes em
 * memória, escolhe como vítima a que terá o próximo acesso mais
 * distante no futuro (ou que nunca mais será acessada). Como exige
 * conhecimento do futuro, só é possível simulá-lo porque o vetor
 * completo de acessos já foi carregado em memória por carregar_log().
 *
 * Para evitar varrer o vetor de acessos inteiro a cada page fault,
 * pré-computamos (em otimo_precalcular_proximos_acessos), para cada
 * posição i do vetor, o índice da próxima ocorrência futura da
 * mesma página (ou -1 se não houver), em O(n) com uma única
 * varredura de trás para frente.
 *
 * Integrantes do grupo:
 *   Rafael Prates       - 2210234
 *   Thiago Coqueiro     - (matrícula pendente)
 *   Guilherme Gratz     - (matrícula pendente)
 *
 * Como cada quadro guarda em last_access o índice (= "time") do
 * acesso que o trouxe/usou por último, e como o "time" do simulador
 * avança exatamente 1 unidade por acesso processado (coincidindo
 * com o índice no vetor), basta consultar
 * proximo_acesso[quadro.last_access] para saber quando aquela
 * página voltará a ser usada.
 */

#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include "alg_otimo.h"
#include "memsim.h"

static long *proximo_acesso = NULL; /* proximo_acesso[i] = próximo índice
                                        com a mesma página de vetor[i],
                                        ou -1 se não houver. */
static int total_acessos = 0;

/* Pré-processa o vetor de acessos completo, calculando para cada
 * posição a próxima ocorrência futura da mesma página. */
static void otimo_preparar(Acesso *vetor_acessos, int n) {
    total_acessos = n;

    free(proximo_acesso);
    proximo_acesso = malloc((size_t)n * sizeof(long));
    if (proximo_acesso == NULL) {
        return;
    }

    /* Descobre a maior página referenciada para dimensionar o vetor
     * auxiliar "ultima_ocorrencia_da_pagina". */
    unsigned int maior_pagina = 0;
    for (int i = 0; i < n; i++) {
        if (vetor_acessos[i].pagina > maior_pagina) {
            maior_pagina = vetor_acessos[i].pagina;
        }
    }

    long *ultima_ocorrencia_da_pagina =
        malloc(((size_t)maior_pagina + 1) * sizeof(long));
    if (ultima_ocorrencia_da_pagina == NULL) {
        free(proximo_acesso);
        proximo_acesso = NULL;
        return;
    }
    for (unsigned int p = 0; p <= maior_pagina; p++) {
        ultima_ocorrencia_da_pagina[p] = -1;
    }

    for (int i = n - 1; i >= 0; i--) {
        unsigned int pagina = vetor_acessos[i].pagina;
        proximo_acesso[i] = ultima_ocorrencia_da_pagina[pagina];
        ultima_ocorrencia_da_pagina[pagina] = i;
    }

    free(ultima_ocorrencia_da_pagina);
}

/* Entre as páginas presentes (quadros ocupados), escolhe a de
 * próximo acesso mais distante (ou nunca mais acessada). */
static int otimo_escolher_vitima(void) {
    Frame *quadros = obter_quadros();
    int num_quadros = obter_num_quadros();

    int melhor = -1;
    long melhor_proximo = -1; /* maior índice de próximo acesso visto
                                  até agora (LONG_MAX representa "nunca
                                  mais será acessada"). */

    for (int i = 0; i < num_quadros; i++) {
        if (!quadros[i].ocupado) {
            continue;
        }

        long idx_ultimo_acesso = quadros[i].last_access;
        long proximo;

        if (proximo_acesso == NULL || idx_ultimo_acesso < 0 ||
            idx_ultimo_acesso >= total_acessos) {
            proximo = LONG_MAX;
        } else {
            long p = proximo_acesso[idx_ultimo_acesso];
            proximo = (p == -1) ? LONG_MAX : p;
        }

        if (melhor == -1 || proximo > melhor_proximo) {
            melhor = i;
            melhor_proximo = proximo;
        }

        /* Página que nunca mais será acessada: já é a melhor vítima
         * possível, pode parar de procurar. */
        if (proximo == LONG_MAX) {
            break;
        }
    }

    return melhor;
}

static AlgoritmoSubstituicao algoritmo_otimo = {
    .inicializar = NULL,
    .preparar = otimo_preparar,
    .escolher_vitima = otimo_escolher_vitima,
    .notificar_acesso = NULL,
    .notificar_carga = NULL,
    .tick = NULL
};

AlgoritmoSubstituicao *obter_algoritmo_otimo(void) {
    return &algoritmo_otimo;
}
