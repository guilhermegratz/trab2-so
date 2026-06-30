# Makefile - sim-virtual (Trabalho 2 de SO, INF1316)
#
# Uso:
#   make            -> compila o simulador (gera ./sim-virtual ou sim-virtual.exe)
#   make clean      -> remove o executavel
#   make simulacoes -> roda todas as combinacoes e gera resultados.csv
#                      (requer os arquivos arquivos/*.log, NAO incluidos no kit)

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
TARGET  = sim-virtual
SRCS    = main.c memsim.c alg_lru.c alg_nru.c alg_clock.c alg_otimo.c
HDRS    = memsim.h algoritmo.h alg_lru.h alg_nru.h alg_clock.h alg_otimo.h

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

simulacoes: $(TARGET)
	./run_simulacoes.sh

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: clean simulacoes
