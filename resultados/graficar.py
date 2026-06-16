"""Genera PNG de comparacion solver vs NSGA-II desde los CSV de C++."""
import csv, glob, os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

def load(p):
    out = []
    with open(p) as f:
        r = csv.reader(f); next(r)
        for row in r:
            if row: out.append((float(row[0]), float(row[1])))
    return out

solver = {
    "A": [(52900.75, 162.0)],
    "B": [(78592.13, 1843.0), (75341.31, 1856.0), (75047.94, 1871.0), (73127.06, 2077.0)],
    "C": [(108221.65, 1.0), (96690.81, 44.0), (75047.94, 1871.0), (73127.06, 2077.0)],
}
for c in ["A", "B", "C"]:
    agg = sorted(load("frente_%s_agregado.csv" % c))
    union = [p for f in glob.glob("frente_%s_semilla*.csv" % c) for p in load(f)]
    plt.figure(figsize=(7, 5))
    if union:
        plt.scatter([p[0] for p in union], [p[1] for p in union], s=12,
                    c="0.75", label="Soluciones por semilla", zorder=1)
    sv = sorted(solver[c])
    plt.plot([p[0] for p in agg], [p[1] for p in agg], "o--", color="tab:blue",
             ms=6, label="NSGA-II (agregado)", zorder=3)
    plt.plot([p[0] for p in sv], [p[1] for p in sv], "s-", color="tab:red",
             ms=8, label="Solver (Gurobi)", zorder=4)
    plt.xlabel("$f_1$ — costo total"); plt.ylabel("$f_2 = \\Delta$ — desbalance")
    plt.title("Caso %s — Solver vs NSGA-II (C++)" % c)
    plt.legend(); plt.grid(True, alpha=0.3); plt.tight_layout()
    plt.savefig("frente_%s.png" % c, dpi=130); plt.close()
    print("frente_%s.png  (agg=%d, nube=%d)" % (c, len(agg), len(union)))
