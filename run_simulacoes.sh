#!/usr/bin/env bash
# run_simulacoes.sh - executa TODAS as combinacoes exigidas pelo enunciado
# (4 logs x 4 algoritmos x {4,8} KB x {1,2,4} MB = 96 simulacoes) e grava
# os resultados em resultados.csv.
#
# Pre-requisitos: ./sim-virtual compilado (make) e os arquivos arquivos/*.log
# (estes NAO fazem parte do kit de entrega).

set -euo pipefail

BIN=./sim-virtual
[ -x "$BIN" ] || BIN=./sim-virtual.exe
OUT=resultados.csv

LOGS="compilador compressor matriz simulador"
ALGOS="LRU NRU Relogio Otimo"
PAGINAS="4 8"
MEMORIAS="1 2 4"

echo "programa,algoritmo,pagina_kb,memoria_mb,page_faults,paginas_escritas" > "$OUT"
for log in $LOGS; do
  for alg in $ALGOS; do
    for pg in $PAGINAS; do
      for mem in $MEMORIAS; do
        res=$("$BIN" "$alg" "arquivos/$log.log" "$pg" "$mem")
        pf=$(echo "$res" | grep "Faltas"   | grep -o '[0-9]*')
        pw=$(echo "$res" | grep "Escritas" | grep -o '[0-9]*')
        echo "$log,$alg,$pg,$mem,$pf,$pw" >> "$OUT"
      done
    done
  done
done
echo "OK -> $OUT"
