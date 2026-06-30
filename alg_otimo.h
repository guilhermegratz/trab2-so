#ifndef ALG_OTIMO_H
#define ALG_OTIMO_H

#include "algoritmo.h"

/* Retorna a instância (estática) da estratégia de substituição
 * Ótima (substitui a página cujo próximo acesso está mais distante
 * no futuro, ou que nunca mais será acessada). */
AlgoritmoSubstituicao *obter_algoritmo_otimo(void);

#endif /* ALG_OTIMO_H */
