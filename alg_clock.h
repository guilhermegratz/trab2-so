#ifndef ALG_CLOCK_H
#define ALG_CLOCK_H

#include "algoritmo.h"

/* Retorna a instância (estática) da estratégia de substituição do
 * Relógio (Clock / Second Chance circular). */
AlgoritmoSubstituicao *obter_algoritmo_clock(void);

#endif /* ALG_CLOCK_H */
