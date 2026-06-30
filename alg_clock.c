/*
 * alg_clock.c
 *
 * Algoritmo do Relógio (Clock): mantém um ponteiro circular sobre os
 * quadros de página. A cada page fault, avança o ponteiro
 * verificando o bit R do quadro apontado: se R=1, zera o bit e
 * avança (dando uma "segunda chance" à página); se R=0, a página
 * daquele quadro é escolhida como vítima.
 */

#include <stddef.h>
#include "alg_clock.h"
#include "memsim.h"

static int ponteiro = 0;

static void clock_inicializar(int num_quadros) {
    (void)num_quadros;
    ponteiro = 0;
}

static int clock_escolher_vitima(void) {
    Frame *quadros = obter_quadros();
    int num_quadros = obter_num_quadros();

    if (num_quadros <= 0) {
        return -1;
    }

    /* Como esta função só é chamada quando a memória está cheia, há
     * garantidamente um quadro ocupado com R=0 dentro de no máximo
     * duas voltas completas. */
    while (1) {
        if (quadros[ponteiro].ocupado) {
            if (quadros[ponteiro].R == 0) {
                int vitima = ponteiro;
                ponteiro = (ponteiro + 1) % num_quadros;
                return vitima;
            }
            quadros[ponteiro].R = 0;
        }
        ponteiro = (ponteiro + 1) % num_quadros;
    }
}

static AlgoritmoSubstituicao algoritmo_clock = {
    .inicializar = clock_inicializar,
    .preparar = NULL,
    .escolher_vitima = clock_escolher_vitima,
    .notificar_acesso = NULL,
    .notificar_carga = NULL,
    .tick = NULL
};

AlgoritmoSubstituicao *obter_algoritmo_clock(void) {
    return &algoritmo_clock;
}
