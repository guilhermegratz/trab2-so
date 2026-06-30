/*
 * main.c
 *
 * sim-virtual - Simulador de memória virtual (paginação) para o
 * Segundo Trabalho de Sistemas Operacionais - INF1316.
 *
 * Integrantes do grupo:
 *   Rafael Prates       - 2210234
 *   Thiago Coqueiro     - (matrícula pendente)
 *   Guilherme Gratz     - (matrícula pendente)
 *
 * Uso:
 *   sim-virtual <algoritmo> <arquivo.log> <tam_pagina_KB> <tam_memoria_MB>
 *
 *   algoritmo:      LRU | NRU | Relogio | Otimo
 *   arquivo.log:    arquivo com a sequência de acessos (endereço hex + R/W)
 *   tam_pagina_KB:  4 ou 8
 *   tam_memoria_MB: 1, 2 ou 4
 *
 * Exemplo:
 *   sim-virtual LRU arquivo.log 8 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "memsim.h"
#include "algoritmo.h"
#include "alg_lru.h"
#include "alg_nru.h"
#include "alg_clock.h"
#include "alg_otimo.h"

/* Valida e converte os 4 parâmetros de linha de comando. Retorna 0
 * em caso de sucesso, ou -1 em caso de erro (já reportado em stderr). */
static int parse_args(int argc, char **argv, Config *config) {
    if (argc != 5) {
        fprintf(stderr,
                "Uso: %s <algoritmo> <arquivo.log> <tam_pagina_KB> <tam_memoria_MB>\n"
                "  algoritmo:      LRU | NRU | Relogio | Otimo\n"
                "  tam_pagina_KB:  4 ou 8\n"
                "  tam_memoria_MB: 1, 2 ou 4\n",
                argv[0]);
        return -1;
    }

    strncpy(config->algoritmo, argv[1], sizeof(config->algoritmo) - 1);
    config->algoritmo[sizeof(config->algoritmo) - 1] = '\0';

    strncpy(config->arquivo, argv[2], sizeof(config->arquivo) - 1);
    config->arquivo[sizeof(config->arquivo) - 1] = '\0';

    config->tam_pagina_kb = atoi(argv[3]);
    config->tam_memoria_mb = atoi(argv[4]);

    if (config->tam_pagina_kb != 4 && config->tam_pagina_kb != 8) {
        fprintf(stderr, "Erro: tamanho de pagina invalido (%d). Use 4 ou 8 KB.\n",
                config->tam_pagina_kb);
        return -1;
    }

    if (config->tam_memoria_mb != 1 && config->tam_memoria_mb != 2 &&
        config->tam_memoria_mb != 4) {
        fprintf(stderr, "Erro: tamanho de memoria invalido (%d). Use 1, 2 ou 4 MB.\n",
                config->tam_memoria_mb);
        return -1;
    }

    return 0;
}

/* Lê todas as linhas do arquivo de log (endereço hex + R/W) para um
 * vetor dinâmico de Acesso. Necessário ter o vetor completo em
 * memória, em especial para o algoritmo Ótimo, e simplifica os
 * demais algoritmos. Retorna NULL em caso de erro. */
static Acesso *carregar_log(const char *filename, int *n_out) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir arquivo de log '%s'\n", filename);
        return NULL;
    }

    int capacidade = 4096;
    int n = 0;
    Acesso *vetor = malloc((size_t)capacidade * sizeof(Acesso));
    if (vetor == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria\n");
        fclose(file);
        return NULL;
    }

    unsigned int addr;
    char rw;
    while (fscanf(file, "%x %c", &addr, &rw) == 2) {
        if (n >= capacidade) {
            capacidade *= 2;
            Acesso *novo = realloc(vetor, (size_t)capacidade * sizeof(Acesso));
            if (novo == NULL) {
                fprintf(stderr, "Erro de alocacao de memoria\n");
                free(vetor);
                fclose(file);
                return NULL;
            }
            vetor = novo;
        }
        vetor[n].addr = addr;
        vetor[n].rw = (char)((rw == 'w') ? 'W' : (rw == 'r') ? 'R' : rw);
        vetor[n].pagina = 0; /* calculado depois, quando s for conhecido */
        n++;
    }

    fclose(file);
    *n_out = n;
    return vetor;
}

/* Seleciona a estratégia de substituição de páginas a partir do
 * nome informado na linha de comando. Aceita variações comuns de
 * grafia ("Relogio"/"Relógio", "Otimo"/"Ótimo"/"OPT"). */
static AlgoritmoSubstituicao *selecionar_algoritmo(const char *nome) {
    if (strcasecmp(nome, "LRU") == 0) {
        return obter_algoritmo_lru();
    }
    if (strcasecmp(nome, "NRU") == 0) {
        return obter_algoritmo_nru();
    }
    if (strcasecmp(nome, "Relogio") == 0 || strcasecmp(nome, "Relógio") == 0 ||
        strcasecmp(nome, "Clock") == 0) {
        return obter_algoritmo_clock();
    }
    if (strcasecmp(nome, "Otimo") == 0 || strcasecmp(nome, "Ótimo") == 0 ||
        strcasecmp(nome, "OPT") == 0) {
        return obter_algoritmo_otimo();
    }
    return NULL;
}

/* Loop principal: para cada acesso, trata o acesso (fault ou não) e
 * avança o contador de tempo. */
static void simular(Acesso *vetor_acessos, int n) {
    for (long time = 0; time < n; time++) {
        tratar_acesso(vetor_acessos[time].pagina, vetor_acessos[time].rw, time);
    }
}

/* Imprime o relatório final no formato especificado no enunciado. */
static void imprimir_relatorio(const Config *config, const Stats *stats) {
    printf("Executando o simulador...\n");
    printf("Arquivo de entrada: %s\n", config->arquivo);
    printf("Tamanho da memoria fisica: %d MB\n", config->tam_memoria_mb);
    printf("Tamanho das paginas: %d KB\n", config->tam_pagina_kb);
    printf("Algoritmo de substituicao: %s\n", config->algoritmo);
    printf("Numero de Faltas de Paginas: %ld\n", stats->page_faults);
    printf("Numero de Paginas Escritas: %ld\n", stats->paginas_escritas);
}

int main(int argc, char **argv) {
    Config config;

    if (parse_args(argc, argv, &config) != 0) {
        return EXIT_FAILURE;
    }

    int n = 0;
    Acesso *vetor_acessos = carregar_log(config.arquivo, &n);
    if (vetor_acessos == NULL) {
        return EXIT_FAILURE;
    }

    AlgoritmoSubstituicao *algo = selecionar_algoritmo(config.algoritmo);
    if (algo == NULL) {
        fprintf(stderr, "Erro: algoritmo de substituicao desconhecido '%s'\n",
                config.algoritmo);
        free(vetor_acessos);
        return EXIT_FAILURE;
    }

    /* Cálculo de s (bits de deslocamento) e da página de cada acesso. */
    int s = calcular_bits_offset(config.tam_pagina_kb);
    for (int i = 0; i < n; i++) {
        vetor_acessos[i].pagina = vetor_acessos[i].addr >> s;
    }

    /* Endereços de 32 bits: o número de páginas possíveis é
     * 2^(32-s). Suficiente para os tamanhos de página exigidos
     * (4KB/8KB) sem estourar um inteiro. */
    long num_paginas_max = 1L << (32 - s);
    inicializar_tabela_paginas((int)num_paginas_max);

    long tam_pagina_bytes = (long)config.tam_pagina_kb * 1024;
    long tam_memoria_bytes = (long)config.tam_memoria_mb * 1024 * 1024;
    int num_quadros = (int)(tam_memoria_bytes / tam_pagina_bytes);
    inicializar_quadros(num_quadros);

    memsim_set_algoritmo(algo);

    if (algo->preparar != NULL) {
        algo->preparar(vetor_acessos, n);
    }
    if (algo->inicializar != NULL) {
        algo->inicializar(num_quadros);
    }

    simular(vetor_acessos, n);

    imprimir_relatorio(&config, obter_stats());

    free(vetor_acessos);
    return EXIT_SUCCESS;
}
