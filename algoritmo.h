#ifndef ALGORITMO_H
#define ALGORITMO_H

#include "memsim.h"

/*
 * algoritmo.h
 *
 * Interface comum usada por main.c/memsim.c para chamar o algoritmo
 * de substituição de páginas escolhido na linha de comando, sem
 * espalhar if/else pelo restante do código (padrão "strategy").
 *
 * Cada algoritmo (LRU, NRU, Relógio, Ótimo) expõe uma instância
 * estática desta struct através de uma função obter_algoritmo_xxx().
 */
typedef struct AlgoritmoSubstituicao {
    /* Inicialização específica do algoritmo (ex.: zerar ponteiro do
     * relógio). Pode ser NULL se o algoritmo não precisar. */
    void (*inicializar)(int num_quadros);

    /* Pré-processamento que precise do vetor completo de acessos
     * (usado apenas pelo algoritmo Ótimo). Pode ser NULL. */
    void (*preparar)(Acesso *vetor_acessos, int n);

    /* Escolhe o índice do quadro vítima dentre os quadros ocupados. */
    int (*escolher_vitima)(void);

    /* Notifica o algoritmo de que o quadro foi acessado (R/W) no
     * instante "time". Pode ser NULL se o algoritmo não precisar
     * manter estado próprio (ex.: NRU já usa os bits R/M do Frame). */
    void (*notificar_acesso)(int quadro, long time);

    /* Notifica o algoritmo de que uma nova página foi carregada no
     * quadro "quadro" no instante "time". */
    void (*notificar_carga)(int quadro, long time);

    /* Chamado a cada acesso processado, antes de tratar_acesso fazer
     * qualquer outra coisa; usado pelo NRU para resetar o bit R
     * periodicamente. Pode ser NULL. */
    void (*tick)(long time);
} AlgoritmoSubstituicao;

#endif /* ALGORITMO_H */
