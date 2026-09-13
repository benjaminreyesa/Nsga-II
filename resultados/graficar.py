"""Genera PNG (vista rapida) y PDF (paper) de comparacion solver vs NSGA-II desde los CSV de C++."""
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

# frentes exactos de referencia (casoPmulti, 11 ponderadores, AMPL/Gurobi)
solver = {
    "A": [(52900.75, 162.0)],
    "B": [(78592.13, 1843.0), (76238.18, 1851.0), (75341.30, 1856.0), (75047.93, 1871.0), (73127.06, 2077.0)],
    "C": [(105923.58, 1.0), (96690.81, 44.0), (94750.38, 140.0), (73127.06, 2077.0)],
}
DESC = {"A": "1 camion, 1 deposito", "B": "2 camiones, 2 depositos", "C": "3 camiones, 3 depositos"}
AMPL = (131/255, 21/255, 104/255)   # AmplColor de la plantilla
NSGA = (138/255, 162/255, 52/255)   # NSGAColor de la plantilla

for c in ["A", "B", "C"]:
    agg = sorted(load("frente_%s_agregado.csv" % c))
    union = [p for f in glob.glob("frente_%s_semilla*.csv" % c) for p in load(f)]
    sv = sorted(solver[c])
    for fmt, size, dpi in (("png", (7, 5), 130), ("pdf", (3.4, 2.5), 300)):
        small = fmt == "pdf"
        fig, ax = plt.subplots(figsize=size, dpi=dpi)
        if union:
            ax.scatter([p[0] / 1000 for p in union], [p[1] for p in union], s=6 if small else 12,
                       color="0.72", label="Soluciones por semilla", zorder=1)
        ax.plot([p[0] / 1000 for p in agg], [p[1] for p in agg], "o--", color=NSGA,
                ms=3.5 if small else 6, lw=0.9, label="NSGA-II (agregado)", zorder=3)
        ax.plot([p[0] / 1000 for p in sv], [p[1] for p in sv], "s-", color=AMPL,
                ms=4 if small else 8, lw=0.9, label="Solver (AMPL/Gurobi)", zorder=4)
        fs = 8 if small else 10
        ax.set_xlabel(r"$f_1$ (costo total, $\times 10^{3}$)", fontsize=fs)
        ax.set_ylabel(r"$f_2 = \Delta$", fontsize=fs)
        ax.tick_params(labelsize=7 if small else 9)
        if not small:
            ax.set_title("Caso %s (%s): solver vs NSGA-II" % (c, DESC[c]))
        ax.grid(True, color="#dddddd", lw=0.6); ax.set_axisbelow(True)
        for s in ("top", "right"): ax.spines[s].set_visible(False)
        ax.legend(fontsize=6.5 if small else 9, frameon=False)
        fig.tight_layout(pad=0.3)
        fig.savefig("comparacion_Caso%s.%s" % (c, fmt))
        plt.close(fig)
    print("comparacion_Caso%s.{png,pdf}  (agregado=%d, nube=%d)" % (c, len(agg), len(union)))
