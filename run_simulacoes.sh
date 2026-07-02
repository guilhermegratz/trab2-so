#!/usr/bin/env bash
# run_simulacoes.sh - executa TODAS as combinacoes exigidas pelo enunciado
# (4 logs x 4 algoritmos x {4,8} KB x {1,2,4} MB = 96 simulacoes),
# imprime os resultados no terminal e grava em resultados.csv.
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

# Cabecalho do CSV
echo "programa,algoritmo,pagina_kb,memoria_mb,page_faults,paginas_escritas" > "$OUT"

# Cabecalho da tabela no terminal
SEP=$(printf '%0.s-' {1..65})
printf "\n%s\n" "$SEP"
printf "%-14s %-10s %7s %8s %13s %12s\n" \
    "PROGRAMA" "ALGORITMO" "PAG(KB)" "MEM(MB)" "PAGE_FAULTS" "ESCRITAS"
printf "%s\n" "$SEP"

ultimo_log=""
for log in $LOGS; do
  for alg in $ALGOS; do
    for pg in $PAGINAS; do
      for mem in $MEMORIAS; do
        res=$("$BIN" "$alg" "arquivos/$log.log" "$pg" "$mem")
        pf=$(echo "$res" | grep "Faltas"   | grep -o '[0-9]*')
        pw=$(echo "$res" | grep "Escritas" | grep -o '[0-9]*')

        # Linha separadora entre programas
        if [ "$log" != "$ultimo_log" ] && [ -n "$ultimo_log" ]; then
            printf "%s\n" "$SEP"
        fi
        ultimo_log="$log"

        # Imprime no terminal
        printf "%-14s %-10s %7s %8s %13s %12s\n" \
            "$log" "$alg" "${pg}KB" "${mem}MB" "$pf" "$pw"

        # Grava no CSV
        echo "$log,$alg,$pg,$mem,$pf,$pw" >> "$OUT"
      done
    done
  done
done

printf "%s\n\n" "$SEP"
echo "96 simulacoes concluidas -> $OUT"
