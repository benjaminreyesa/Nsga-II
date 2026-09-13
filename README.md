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
mingw32-make          # Windows;  en Linux: make  (o: g++ -O2 -std=c++17 -o nsga2 instancia.cpp nsga2.cpp main.cpp)
./nsga2               # 10 semillas, popsize=100, ngen=250, casos A,B,C
./nsga2 --rapido      # version corta (3 semillas, pop 80, 120 gen)
./nsga2 --casos B,C --pop 100 --gen 250 --semillas 10   # parametros explicitos
cd resultados && python graficar.py                     # figuras comparacion_Caso*.png/pdf
```
Se ejecuta desde esta carpeta (las instancias se leen de `data/`).

## Parámetros
popsize=100 (múltiplo de 4), generaciones=250, prob. cruzamiento=0.9,
prob. mutación=0.3 (por hijo), torneo binario, semillas 1..10.

## Fidelidad al NSGA-II de referencia (Deb v1.1.6, código del curso)
- Manejo de infactibilidad por **dominancia restringida** (`domina_ind`, como
  `check_dominance` de `dominance.c`): factible domina a infactible; entre
  infactibles gana la menor violación; entre factibles, dominancia de Pareto.
  No se penalizan los objetivos.
- Torneo binario como `tourselect.c`: dominancia restringida directa, luego
  distancia de aglomeración y, en empate, azar.
- Selección como `selection()`: dos permutaciones de la población, cada
  individuo participa en dos torneos, cada cruce produce dos hijos.
- Distancia de aglomeración promediada sobre los objetivos (`crowddist.c`) y
  conservada desde el ordenamiento de la población mezclada (`fillnds.c`).
- Propio del problema: representación entera con separadores, inicialización
  híbrida, operadores de ruta con reparación.

## Salida (`resultados/`)
- `frente_<caso>_semillaNN.csv`: frente de cada semilla.
- `frente_<caso>_agregado.csv`: frente agregado de las 10 semillas.
- `resumen_semillas.csv`: puntos, mejor costo, mejor desbalance y tiempo por semilla.
- `resumen_indicadores.csv`: hipervolumen y two-set coverage por caso.
- `comparacion_Caso<X>.png/.pdf`: frente exacto vs frente agregado NSGA-II.
