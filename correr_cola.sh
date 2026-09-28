#!/bin/bash
# Corre NSGA-II en cola sobre todas las instancias .dat de una carpeta, una por una,
# de menor a mayor numero de puntos. Cada instancia deja sus CSV en
# resultados_cola/<instancia>/ y la salida de consola en resultados_cola/<instancia>.log.
# Si una instancia ya tiene su frente agregado se salta, asi la cola se puede relanzar.
#
# Uso (desde la carpeta del repo, con ./nsga2 compilado):
#   ./correr_cola.sh <carpeta_con_.dat> [semillas] [pop] [gen] [carpeta_salida]
#   nohup ./correr_cola.sh ~/experimentos/Instancias > cola.log 2>&1 &
#   nohup ./correr_cola.sh ~/experimentos/Instancias 10 200 1000 resultados_cola_200x1000 > cola_200x1000.log 2>&1 &

DIR=${1:?falta la carpeta con las instancias .dat}
SEM=${2:-10}
POP=${3:-100}
GEN=${4:-250}
OUT=${5:-resultados_cola}

BIN=./nsga2
[ -x "$BIN" ] || BIN=./nsga2.exe
mkdir -p "$OUT" resultados

# lista ordenada por puntos (el numero que sigue a "Instance_p"); se ignoran los _multi
LISTA=$(ls "$DIR"/*.dat | grep -v _multi \
        | sed -E 's|^(.*Instance_p)([0-9]+)(_.*)$|\2 \1\2\3|' | sort -n | cut -d' ' -f2-)

echo "[$(date '+%F %T')] cola: $(echo "$LISTA" | wc -l) instancias, semillas=$SEM pop=$POP gen=$GEN"
for f in $LISTA; do
  n=$(basename "$f" .dat)
  if [ -f "$OUT/$n/frente_${n}_agregado.csv" ]; then
    echo "[$(date '+%T')] $n ya tiene resultados, se salta"
    continue
  fi
  echo "[$(date '+%T')] inicio  $n"
  "$BIN" --instancias "$f" --semillas "$SEM" --pop "$POP" --gen "$GEN" > "$OUT/$n.log" 2>&1
  mkdir -p "$OUT/$n"
  mv resultados/frente_"$n"_*.csv resultados/resumen_indicadores.csv resultados/resumen_semillas.csv "$OUT/$n/" 2>/dev/null
  echo "[$(date '+%T')] fin     $n  $(grep -m1 'frente agregado' "$OUT/$n.log")"
done
echo "[$(date '+%F %T')] COLA TERMINADA"
