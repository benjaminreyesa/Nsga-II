# NSGA-II — Recolección de residuos reciclables (Entrega 3)

NSGA-II adaptado al problema biobjetivo de recolección multi-depósito y
multi-compartimento. Se compara contra el frente exacto obtenido con AMPL/Gurobi.

## Archivos
- `instancia.hpp` / `instancia.cpp`: lectura de las instancias `.dat`.
- `nsga2.hpp` / `nsga2.cpp`: representación, evaluación, inicialización, operadores,
  núcleo NSGA-II e indicadores.
- `main.cpp`: ejecuta las 10 semillas en los 3 casos y escribe los resultados.
- `data/`: las tres instancias de prueba.
- `resultados/`: salidas (CSV) y `graficar.py` para los gráficos.

## Compilar y ejecutar
```bash
mingw32-make          # o: g++ -O2 -std=c++17 -o nsga2 instancia.cpp nsga2.cpp main.cpp
./nsga2               # 10 semillas, popsize=100, ngen=250
./nsga2 --rapido      # version corta (3 semillas)
```

## Parámetros
popsize=100, generaciones=250, prob. cruzamiento=0.9, prob. mutación=0.3,
torneo binario, semillas 1..10.

## Salida (`resultados/`)
- `frente_<caso>_semillaNN.csv`: frente de cada semilla.
- `frente_<caso>_agregado.csv`: frente agregado de las 10 semillas.
- `resumen_indicadores.csv`: hipervolumen y two-set coverage por caso.
