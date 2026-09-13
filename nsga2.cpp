#include "nsga2.hpp"
#include <set>
#include <unordered_set>
#include <cmath>
#include <limits>

static const double INF = std::numeric_limits<double>::infinity();

// =========================== REPRESENTACION ===========================
// Vector entero de longitud variable con separadores negativos.
//   positivos: puntos de recoleccion ; negativos -1..-V: fin de ruta del vehiculo.
//   El vehiculo j (1-indexado) opera desde inst.vehicle_map[j-1].

Rutas decode(const std::vector<int>& crom, const Instancia& inst) {
    int V = inst.n_vehiculos;
    Rutas rutas(V);
    int v = 0;
    for (int tok : crom) {
        if (tok < 0) {
            ++v;
            if (v >= V) v = V - 1;     // tras el ultimo separador no deberia haber puntos
        } else {
            rutas[v].push_back(tok);
        }
    }
    return rutas;
}

std::vector<int> codifica(const Rutas& rutas, const Instancia& inst) {
    std::vector<int> crom;
    for (int j = 0; j < inst.n_vehiculos; ++j) {
        for (int p : rutas[j]) crom.push_back(p);
        crom.push_back(-(j + 1));
    }
    return crom;
}

bool es_valido(const std::vector<int>& crom, const Instancia& inst) {
    std::vector<int> pos, neg;
    for (int t : crom) (t > 0 ? pos : neg).push_back(t);
    std::vector<int> nsort = inst.N;
    std::sort(nsort.begin(), nsort.end());
    std::sort(pos.begin(), pos.end());
    if (pos != nsort) return false;
    std::vector<int> esperado;
    for (int j = 0; j < inst.n_vehiculos; ++j) esperado.push_back(-(j + 1));
    return neg == esperado;
}

std::vector<int> repara(const std::vector<int>& crom, const Instancia& inst, RNG& rng) {
    int V = inst.n_vehiculos;
    // 1) partir en segmentos por separadores
    Rutas segmentos(1);
    for (int tok : crom) {
        if (tok < 0) segmentos.push_back({});
        else segmentos.back().push_back(tok);
    }
    // 2) ajustar a V segmentos
    if ((int)segmentos.size() > V) {
        std::vector<int> extra;
        for (size_t s = V; s < segmentos.size(); ++s)
            for (int p : segmentos[s]) extra.push_back(p);
        segmentos.resize(V);
        for (int p : extra) segmentos[V - 1].push_back(p);
    }
    while ((int)segmentos.size() < V) segmentos.push_back({});
    // 3) limpiar repetidos y detectar faltantes
    std::unordered_set<int> enN(inst.N.begin(), inst.N.end());
    std::unordered_set<int> vistos;
    for (auto& s : segmentos) {
        std::vector<int> limpio;
        for (int p : s)
            if (enN.count(p) && !vistos.count(p)) { vistos.insert(p); limpio.push_back(p); }
        s = limpio;
    }
    std::vector<int> faltantes;
    for (int p : inst.N) if (!vistos.count(p)) faltantes.push_back(p);
    rng.shuffle(faltantes);
    for (int p : faltantes) segmentos[rng.randrange(V)].push_back(p);
    return codifica(segmentos, inst);
}

std::vector<int> individuo_aleatorio(const Instancia& inst, RNG& rng) {
    std::vector<int> puntos = inst.N;
    rng.shuffle(puntos);
    Rutas rutas(inst.n_vehiculos);
    for (int p : puntos) rutas[rng.randrange(inst.n_vehiculos)].push_back(p);
    return codifica(rutas, inst);
}

// ============================= EVALUACION =============================
static double largo_ruta(int dep, const std::vector<int>& ruta, const Instancia& inst) {
    if (ruta.empty()) return 0.0;
    double d = inst.dist(dep, ruta.front());
    for (size_t i = 0; i + 1 < ruta.size(); ++i) d += inst.dist(ruta[i], ruta[i + 1]);
    d += inst.dist(ruta.back(), dep);
    return d;
}

void evalua(Individuo& ind, const Instancia& inst) {
    Rutas rutas = decode(ind.crom, inst);
    double f1 = 0.0, viol = 0.0;
    std::map<int,double> L;
    for (int m : inst.M) L[m] = 0.0;

    for (int j = 0; j < inst.n_vehiculos; ++j) {
        const std::vector<int>& ruta = rutas[j];
        if (ruta.empty()) continue;
        int dep = inst.vehicle_map[j].first;
        int k = inst.vehicle_map[j].second;

        double dist = largo_ruta(dep, ruta, inst);
        double Fk = inst.F.count(k) ? inst.F.at(k) : 0.0;
        double ak = inst.alfa.count(k) ? inst.alfa.at(k) : 0.0;
        f1 += Fk + ak * dist;

        double carga_ruta = 0.0;
        for (int i : ruta) carga_ruta += inst.carga_punto.at(i);
        L[dep] += carga_ruta;

        // capacidad por compartimento
        for (int r : inst.R) {
            double carga_r = 0.0;
            for (int i : ruta) {
                auto it = inst.q.find({i, r});
                if (it != inst.q.end()) carga_r += it->second;
            }
            double cap = inst.Q.count({r, k}) ? inst.Q.at({r, k}) : 0.0;
            double sobra = (1.0 + inst.beta) * carga_r - cap;
            if (sobra > 0) viol += sobra;
        }
        // turno
        double tiempo = inst.gamma * dist + inst.eta * carga_ruta;
        if (tiempo - inst.Tmax > 0) viol += tiempo - inst.Tmax;
    }

    double lmin = INF, lmax = -INF;
    for (auto& kv : L) { lmin = std::min(lmin, kv.second); lmax = std::max(lmax, kv.second); }
    double f2 = lmax - lmin;

    ind.f1 = f1; ind.f2 = f2; ind.viol = viol;
    ind.factible = (viol <= 1e-9);
}

// ============================ INICIALIZACION =========================
static std::vector<int> vehiculos_de_deposito(const Instancia& inst, int dep) {
    std::vector<int> v;
    for (int j = 0; j < inst.n_vehiculos; ++j)
        if (inst.vehicle_map[j].first == dep) v.push_back(j);
    return v;
}

static std::vector<int> ruteo_vecino(int dep, std::vector<int> puntos, const Instancia& inst) {
    std::vector<int> ruta;
    int actual = dep;
    while (!puntos.empty()) {
        int mejor = 0; double md = INF;
        for (size_t i = 0; i < puntos.size(); ++i) {
            double d = inst.dist(actual, puntos[i]);
            if (d < md) { md = d; mejor = (int)i; }
        }
        ruta.push_back(puntos[mejor]);
        actual = puntos[mejor];
        puntos.erase(puntos.begin() + mejor);
    }
    return ruta;
}

std::vector<int> greedy_cercania(const Instancia& inst, RNG&) {
    std::vector<int> activos = inst.depositos_activos();
    std::map<int,std::vector<int>> asign;
    for (int dep : activos) asign[dep] = {};
    for (int p : inst.N) {
        int mejor = activos[0]; double md = INF;
        for (int dep : activos) { double d = inst.dist(dep, p); if (d < md) { md = d; mejor = dep; } }
        asign[mejor].push_back(p);
    }
    Rutas rutas(inst.n_vehiculos);
    for (int dep : activos) {
        std::vector<int> veh = vehiculos_de_deposito(inst, dep);
        std::vector<int> orden = ruteo_vecino(dep, asign[dep], inst);
        for (size_t idx = 0; idx < orden.size(); ++idx)
            rutas[veh[idx % veh.size()]].push_back(orden[idx]);
    }
    return codifica(rutas, inst);
}

std::vector<int> orientada_balance(const Instancia& inst, RNG&) {
    std::vector<int> activos = inst.depositos_activos();
    std::map<int,double> carga;
    std::map<int,std::vector<int>> asign;
    for (int dep : activos) { carga[dep] = 0.0; asign[dep] = {}; }
    std::vector<int> puntos = inst.N;
    std::sort(puntos.begin(), puntos.end(),
              [&](int a, int b){ return inst.carga_punto.at(a) > inst.carga_punto.at(b); });
    for (int p : puntos) {
        int mejor = activos[0]; double mc = INF;
        for (int dep : activos) if (carga[dep] < mc) { mc = carga[dep]; mejor = dep; }
        asign[mejor].push_back(p);
        carga[mejor] += inst.carga_punto.at(p);
    }
    Rutas rutas(inst.n_vehiculos);
    for (int dep : activos) {
        std::vector<int> veh = vehiculos_de_deposito(inst, dep);
        std::vector<int> orden = ruteo_vecino(dep, asign[dep], inst);
        for (size_t idx = 0; idx < orden.size(); ++idx)
            rutas[veh[idx % veh.size()]].push_back(orden[idx]);
    }
    return codifica(rutas, inst);
}

std::vector<std::vector<int>> inicializa_poblacion(const Instancia& inst,
                                                   int popsize, RNG& rng) {
    std::vector<std::vector<int>> pob;
    int n_greedy = std::max(1, (int)(popsize * 0.15));
    int n_balance = std::max(1, (int)(popsize * 0.15));
    std::vector<int> base_g = greedy_cercania(inst, rng);
    std::vector<int> base_b = orientada_balance(inst, rng);
    pob.push_back(base_g);
    pob.push_back(base_b);
    for (int i = 0; i < n_greedy - 1; ++i) pob.push_back(mutacion(base_g, inst, rng));
    for (int i = 0; i < n_balance - 1; ++i) pob.push_back(mutacion(base_b, inst, rng));
    while ((int)pob.size() < popsize) pob.push_back(individuo_aleatorio(inst, rng));
    pob.resize(popsize);
    return pob;
}

// ============================== OPERADORES ============================
static std::vector<int> rbx(const std::vector<int>& c1, const std::vector<int>& c2,
                            const Instancia& inst, RNG& rng) {
    int V = inst.n_vehiculos;
    Rutas r1 = decode(c1, inst), r2 = decode(c2, inst);
    std::vector<int> cand;
    for (int j = 0; j < V; ++j) if (!r1[j].empty()) cand.push_back(j);
    if (cand.empty()) return c2;
    int j = rng.choice(cand);
    std::unordered_set<int> bloq(r1[j].begin(), r1[j].end());
    Rutas hijo(V);
    hijo[j] = r1[j];
    for (int v = 0; v < V; ++v) {
        if (v == j) continue;
        for (int p : r2[v]) if (!bloq.count(p)) { hijo[v].push_back(p); bloq.insert(p); }
    }
    return repara(codifica(hijo, inst), inst, rng);
}

static std::vector<int> ox(const std::vector<int>& c1, const std::vector<int>& c2,
                           const Instancia& inst, RNG& rng) {
    std::vector<int> s1, s2;
    for (int t : c1) if (t > 0) s1.push_back(t);
    for (int t : c2) if (t > 0) s2.push_back(t);
    int n = (int)s1.size();
    if (n < 2) return c1;
    auto pr = rng.sample2(n);
    int i = std::min(pr.first, pr.second), k = std::max(pr.first, pr.second);
    std::vector<int> hijo(n, 0);
    std::unordered_set<int> tomados;
    for (int t = i; t <= k; ++t) { hijo[t] = s1[t]; tomados.insert(s1[t]); }
    std::vector<int> relleno;
    for (int p : s2) if (!tomados.count(p)) relleno.push_back(p);
    int idx = 0;
    for (int t = 0; t < n; ++t) if (hijo[t] == 0) hijo[t] = relleno[idx++];
    // re-particionar heredando tamanos de ruta del padre 1
    Rutas r1 = decode(c1, inst);
    Rutas rutas(inst.n_vehiculos);
    int pos = 0;
    for (int v = 0; v < inst.n_vehiculos; ++v) {
        int sz = (int)r1[v].size();
        for (int t = 0; t < sz && pos < n; ++t) rutas[v].push_back(hijo[pos++]);
    }
    return repara(codifica(rutas, inst), inst, rng);
}

std::vector<int> cruzamiento(const std::vector<int>& a, const std::vector<int>& b,
                             const Instancia& inst, RNG& rng) {
    if (inst.n_vehiculos == 1 || rng.rand01() < 0.5) return ox(a, b, inst, rng);
    return rbx(a, b, inst, rng);
}

// ---- mutaciones (operan sobre Rutas) ----
static void op_swap(Rutas& r, RNG& rng, const Instancia&) {
    std::vector<std::pair<int,int>> plano;
    for (int v = 0; v < (int)r.size(); ++v)
        for (int i = 0; i < (int)r[v].size(); ++i) plano.push_back({v, i});
    if (plano.size() < 2) return;
    auto pr = rng.sample2((int)plano.size());
    auto a = plano[pr.first], b = plano[pr.second];
    std::swap(r[a.first][a.second], r[b.first][b.second]);
}

static void op_relocate(Rutas& r, RNG& rng, const Instancia&) {
    std::vector<std::pair<int,int>> plano;
    for (int v = 0; v < (int)r.size(); ++v)
        for (int i = 0; i < (int)r[v].size(); ++i) plano.push_back({v, i});
    if (plano.empty()) return;
    auto pos = rng.choice(plano);
    int punto = r[pos.first][pos.second];
    r[pos.first].erase(r[pos.first].begin() + pos.second);
    int dest = rng.randrange((int)r.size());
    int p = rng.randint(0, (int)r[dest].size());
    r[dest].insert(r[dest].begin() + p, punto);
}

static void op_dos_opt(Rutas& rutas, RNG& rng, const Instancia& inst) {
    // 2-opt de mejora valido para distancias asimetricas: el delta incluye la
    // inversion de las aristas internas del tramo.
    std::vector<int> cand;
    for (int v = 0; v < (int)rutas.size(); ++v) if (rutas[v].size() >= 3) cand.push_back(v);
    if (cand.empty()) return;
    int v = rng.choice(cand);
    int dep = inst.vehicle_map[v].first;
    std::vector<int>& R = rutas[v];
    bool mejora = true; int tope = (int)R.size();
    while (mejora && tope > 0) {
        mejora = false; --tope;
        int n = (int)R.size();
        for (int i = 0; i < n - 1 && !mejora; ++i) {
            int left = (i == 0) ? dep : R[i - 1];
            for (int k = i + 1; k < n; ++k) {
                int right = (k == n - 1) ? dep : R[k + 1];
                double old_int = 0, new_int = 0;
                for (int m = i; m < k; ++m) {
                    old_int += inst.dist(R[m], R[m + 1]);
                    new_int += inst.dist(R[m + 1], R[m]);
                }
                double viejo = inst.dist(left, R[i]) + old_int + inst.dist(R[k], right);
                double nuevo = inst.dist(left, R[k]) + new_int + inst.dist(R[i], right);
                if (nuevo < viejo - 1e-9) {
                    std::reverse(R.begin() + i, R.begin() + k + 1);
                    mejora = true; break;
                }
            }
        }
    }
}

static void op_or_opt(Rutas& rutas, RNG& rng, const Instancia& inst) {
    std::vector<std::pair<int,int>> plano;
    for (int v = 0; v < (int)rutas.size(); ++v)
        for (int i = 0; i < (int)rutas[v].size(); ++i) plano.push_back({v, i});
    if (plano.empty()) return;
    auto pos = rng.choice(plano);
    int p = rutas[pos.first][pos.second];
    rutas[pos.first].erase(rutas[pos.first].begin() + pos.second);
    int bw = -1, bp = -1; double bc = INF;
    for (int w = 0; w < (int)rutas.size(); ++w) {
        int dep = inst.vehicle_map[w].first;
        std::vector<int>& ru = rutas[w];
        for (int q = 0; q <= (int)ru.size(); ++q) {
            int prev = (q == 0) ? dep : ru[q - 1];
            int nxt = (q == (int)ru.size()) ? dep : ru[q];
            double delta = inst.dist(prev, p) + inst.dist(p, nxt) - inst.dist(prev, nxt);
            if (delta < bc) { bc = delta; bw = w; bp = q; }
        }
    }
    rutas[bw].insert(rutas[bw].begin() + bp, p);
}

static void op_segmento(Rutas& rutas, RNG& rng, const Instancia&) {
    std::vector<int> cand;
    for (int v = 0; v < (int)rutas.size(); ++v) if (!rutas[v].empty()) cand.push_back(v);
    if (cand.empty()) return;
    int origen = rng.choice(cand);
    std::vector<int>& ru = rutas[origen];
    int i = rng.randrange((int)ru.size());
    int j = rng.randint(i + 1, (int)ru.size());
    std::vector<int> seg(ru.begin() + i, ru.begin() + j);
    ru.erase(ru.begin() + i, ru.begin() + j);
    int dest = rng.randrange((int)rutas.size());
    int p = rng.randint(0, (int)rutas[dest].size());
    rutas[dest].insert(rutas[dest].begin() + p, seg.begin(), seg.end());
}

static void op_vaciar(Rutas& rutas, RNG& rng, const Instancia& inst) {
    std::vector<int> usados;
    for (int v = 0; v < (int)rutas.size(); ++v) if (!rutas[v].empty()) usados.push_back(v);
    if (usados.size() <= 1) return;
    int v = rng.choice(usados);
    std::vector<int> puntos = rutas[v];
    rutas[v].clear();
    std::vector<int> otros;
    for (int w : usados) if (w != v) otros.push_back(w);
    for (int p : puntos) {
        int bw = otros[0], bp = 0; double bc = INF;
        for (int w : otros) {
            int dep = inst.vehicle_map[w].first;
            std::vector<int>& ru = rutas[w];
            for (int q = 0; q <= (int)ru.size(); ++q) {
                int prev = (q == 0) ? dep : ru[q - 1];
                int nxt = (q == (int)ru.size()) ? dep : ru[q];
                double delta = inst.dist(prev, p) + inst.dist(p, nxt) - inst.dist(prev, nxt);
                if (delta < bc) { bc = delta; bw = w; bp = q; }
            }
        }
        rutas[bw].insert(rutas[bw].begin() + bp, p);
    }
}

std::vector<int> mutacion(const std::vector<int>& crom, const Instancia& inst, RNG& rng) {
    Rutas rutas = decode(crom, inst);
    int op = rng.randrange(6);
    switch (op) {
        case 0: op_swap(rutas, rng, inst); break;
        case 1: op_relocate(rutas, rng, inst); break;
        case 2: op_dos_opt(rutas, rng, inst); break;
        case 3: op_or_opt(rutas, rng, inst); break;
        case 4: op_segmento(rutas, rng, inst); break;
        default: op_vaciar(rutas, rng, inst); break;
    }
    std::vector<int> nuevo = codifica(rutas, inst);
    if (!es_valido(nuevo, inst)) nuevo = repara(nuevo, inst, rng);
    return nuevo;
}

// ============================ NUCLEO NSGA-II =========================
static bool domina_ind(const Individuo& a, const Individuo& b) {
    // Dominancia restringida (Deb et al., 2002): la factibilidad manda.
    if (a.factible != b.factible) return a.factible;     // factible domina a infactible
    if (!a.factible) return a.viol < b.viol - 1e-12;     // ambos infactibles: menor violacion
    // ambos factibles: dominancia de Pareto sobre los objetivos originales
    bool mejor = false;
    if (a.f1 > b.f1 || a.f2 > b.f2) return false;
    if (a.f1 < b.f1 || a.f2 < b.f2) mejor = true;
    return mejor;
}

static std::vector<std::vector<int>> fast_nds(std::vector<Individuo>& pop) {
    int n = (int)pop.size();
    std::vector<std::vector<int>> S(n);
    std::vector<int> ndom(n, 0);
    std::vector<std::vector<int>> frentes(1);
    for (int p = 0; p < n; ++p) {
        for (int q = 0; q < n; ++q) {
            if (p == q) continue;
            if (domina_ind(pop[p], pop[q])) S[p].push_back(q);
            else if (domina_ind(pop[q], pop[p])) ndom[p]++;
        }
        if (ndom[p] == 0) { pop[p].rank = 0; frentes[0].push_back(p); }
    }
    int i = 0;
    while (!frentes[i].empty()) {
        std::vector<int> sig;
        for (int p : frentes[i])
            for (int q : S[p])
                if (--ndom[q] == 0) { pop[q].rank = i + 1; sig.push_back(q); }
        ++i;
        frentes.push_back(sig);
    }
    frentes.pop_back();
    return frentes;
}

static void crowding(std::vector<Individuo>& pop, const std::vector<int>& fr) {
    int l = (int)fr.size();
    if (l == 0) return;
    for (int idx : fr) pop[idx].cd = 0.0;
    if (l <= 2) { for (int idx : fr) pop[idx].cd = INF; return; }
    for (int m = 0; m < 2; ++m) {
        std::vector<int> ord = fr;
        std::sort(ord.begin(), ord.end(), [&](int a, int b){
            return (m == 0 ? pop[a].f1 < pop[b].f1 : pop[a].f2 < pop[b].f2);
        });
        pop[ord.front()].cd = INF;
        pop[ord.back()].cd = INF;
        double fmin = (m == 0 ? pop[ord.front()].f1 : pop[ord.front()].f2);
        double fmax = (m == 0 ? pop[ord.back()].f1 : pop[ord.back()].f2);
        if (fmax - fmin < 1e-12) continue;
        for (int k = 1; k < l - 1; ++k) {
            double prev = (m == 0 ? pop[ord[k - 1]].f1 : pop[ord[k - 1]].f2);
            double next = (m == 0 ? pop[ord[k + 1]].f1 : pop[ord[k + 1]].f2);
            pop[ord[k]].cd += (next - prev) / (fmax - fmin);
        }
    }
    // como en crowddist.c de Deb: la distancia se promedia sobre los objetivos
    for (int idx : fr) if (pop[idx].cd != INF) pop[idx].cd /= 2.0;
}

// Torneo binario tal como tourselect.c de Deb: dominancia restringida directa,
// luego distancia de aglomeracion y, en empate, azar.
static const Individuo& torneo(const Individuo& a, const Individuo& b, RNG& rng) {
    if (domina_ind(a, b)) return a;
    if (domina_ind(b, a)) return b;
    if (a.cd > b.cd) return a;
    if (b.cd > a.cd) return b;
    return rng.rand01() <= 0.5 ? a : b;
}

// Descendencia de un par de padres: con probabilidad pcross se cruzan (dos
// hijos, uno por cada orden de los padres); si no, se copian. Cada hijo muta
// con probabilidad pmut.
static void descendencia(const Individuo& p1, const Individuo& p2,
                         const Instancia& inst, RNG& rng, double pcross, double pmut,
                         std::vector<Individuo>& Q) {
    std::vector<int> h1, h2;
    if (rng.rand01() <= pcross) {
        h1 = cruzamiento(p1.crom, p2.crom, inst, rng);
        h2 = cruzamiento(p2.crom, p1.crom, inst, rng);
    } else {
        h1 = p1.crom;
        h2 = p2.crom;
    }
    if (rng.rand01() <= pmut) h1 = mutacion(h1, inst, rng);
    if (rng.rand01() <= pmut) h2 = mutacion(h2, inst, rng);
    Individuo a; a.crom = h1; evalua(a, inst); Q.push_back(a);
    Individuo b; b.crom = h2; evalua(b, inst); Q.push_back(b);
}

// Seleccion tal como selection() de Deb: dos permutaciones de la poblacion,
// cada individuo participa en exactamente dos torneos y cada grupo de cuatro
// produce cuatro hijos.
static std::vector<Individuo> seleccion(const std::vector<Individuo>& P,
                                        const Instancia& inst, RNG& rng,
                                        double pcross, double pmut) {
    int n = (int)P.size();
    std::vector<int> a1(n), a2(n);
    for (int i = 0; i < n; ++i) a1[i] = a2[i] = i;
    rng.shuffle(a1);
    rng.shuffle(a2);
    std::vector<Individuo> Q;
    for (int i = 0; i + 3 < n; i += 4) {
        const Individuo& p1 = torneo(P[a1[i]], P[a1[i + 1]], rng);
        const Individuo& p2 = torneo(P[a1[i + 2]], P[a1[i + 3]], rng);
        descendencia(p1, p2, inst, rng, pcross, pmut, Q);
        const Individuo& p3 = torneo(P[a2[i]], P[a2[i + 1]], rng);
        const Individuo& p4 = torneo(P[a2[i + 2]], P[a2[i + 3]], rng);
        descendencia(p3, p4, inst, rng, pcross, pmut, Q);
    }
    return Q;
}

std::vector<Individuo> nsga2(const Instancia& inst, RNG& rng,
                             int popsize, int ngen, double pcross, double pmut) {
    // como en Deb, la poblacion debe ser multiplo de 4 (grupos de dos torneos)
    popsize -= popsize % 4;
    std::vector<Individuo> P;
    for (auto& c : inicializa_poblacion(inst, popsize, rng)) {
        Individuo ind; ind.crom = c; evalua(ind, inst); P.push_back(ind);
    }
    auto fr = fast_nds(P);
    for (auto& f : fr) crowding(P, f);

    for (int gen = 1; gen < ngen; ++gen) {
        std::vector<Individuo> Q = seleccion(P, inst, rng, pcross, pmut);
        std::vector<Individuo> Rp = P;
        Rp.insert(Rp.end(), Q.begin(), Q.end());
        auto frentes = fast_nds(Rp);
        std::vector<int> nueva;
        size_t i = 0;
        while (i < frentes.size() && nueva.size() + frentes[i].size() <= (size_t)popsize) {
            crowding(Rp, frentes[i]);
            for (int idx : frentes[i]) nueva.push_back(idx);
            ++i;
        }
        if ((int)nueva.size() < popsize && i < frentes.size()) {
            crowding(Rp, frentes[i]);
            std::vector<int> resto = frentes[i];
            std::sort(resto.begin(), resto.end(),
                      [&](int a, int b){ return Rp[a].cd > Rp[b].cd; });
            for (int idx : resto) {
                if ((int)nueva.size() >= popsize) break;
                nueva.push_back(idx);
            }
        }
        // rango y distancia de aglomeracion se conservan tal como quedaron en el
        // ordenamiento de la poblacion mezclada (fillnds.c de Deb)
        std::vector<Individuo> nuevaP;
        for (int idx : nueva) nuevaP.push_back(Rp[idx]);
        P = nuevaP;
    }
    return P;
}

std::vector<std::pair<double,double>> frente_factible(const std::vector<Individuo>& pop) {
    std::vector<std::pair<double,double>> pts;
    for (auto& ind : pop) if (ind.factible) pts.push_back({ind.f1, ind.f2});
    return pts;
}

// ============================= INDICADORES ===========================
static bool domina_pt(const Punto& a, const Punto& b) {
    return (a.first <= b.first && a.second <= b.second) &&
           (a.first < b.first || a.second < b.second);
}

static double redondea(double x) { return std::round(x * 1e6) / 1e6; }

std::vector<Punto> filtra_no_dominados(const std::vector<Punto>& pts) {
    std::set<Punto> unicos;
    for (auto& p : pts) unicos.insert({redondea(p.first), redondea(p.second)});
    std::vector<Punto> u(unicos.begin(), unicos.end());
    std::vector<Punto> nd;
    for (auto& p : u) {
        bool dom = false;
        for (auto& q : u) if (!(q == p) && domina_pt(q, p)) { dom = true; break; }
        if (!dom) nd.push_back(p);
    }
    std::sort(nd.begin(), nd.end());
    return nd;
}

std::vector<Punto> frente_agregado(const std::vector<std::vector<Punto>>& frentes) {
    std::vector<Punto> todos;
    for (auto& fr : frentes) for (auto& p : fr) todos.push_back(p);
    return filtra_no_dominados(todos);
}

static Punto normaliza(const Punto& p, const Punto& lo, const Punto& hi) {
    double dx = hi.first > lo.first ? hi.first - lo.first : 1.0;
    double dy = hi.second > lo.second ? hi.second - lo.second : 1.0;
    return {(p.first - lo.first) / dx, (p.second - lo.second) / dy};
}

double hypervolume_2d(const std::vector<Punto>& frente, Punto ref, Punto lo, Punto hi) {
    if (frente.empty()) return 0.0;
    std::vector<Punto> pts;
    for (auto& p : frente) pts.push_back(normaliza(p, lo, hi));
    Punto r = normaliza(ref, lo, hi);
    std::vector<Punto> dent;
    for (auto& p : pts) if (p.first < r.first && p.second < r.second) dent.push_back(p);
    if (dent.empty()) return 0.0;
    dent = filtra_no_dominados(dent);
    std::sort(dent.begin(), dent.end());
    double hv = 0.0, f2_prev = r.second;
    for (auto& p : dent) { hv += (r.first - p.first) * (f2_prev - p.second); f2_prev = p.second; }
    return hv;
}

double two_set_coverage(const std::vector<Punto>& A, const std::vector<Punto>& B) {
    // tolerancia para que el redondeo no afecte la dominancia debil
    const double EPS = 0.5;
    if (B.empty()) return 0.0;
    int cub = 0;
    for (auto& b : B)
        for (auto& a : A)
            if (a.first <= b.first + EPS && a.second <= b.second + EPS) { ++cub; break; }
    return (double)cub / (double)B.size();
}

void rango_comun(const std::vector<Punto>& a, const std::vector<Punto>& b,
                 Punto& lo, Punto& hi) {
    std::vector<Punto> t = a;
    t.insert(t.end(), b.begin(), b.end());
    lo = {INF, INF}; hi = {-INF, -INF};
    for (auto& p : t) {
        lo.first = std::min(lo.first, p.first); lo.second = std::min(lo.second, p.second);
        hi.first = std::max(hi.first, p.first); hi.second = std::max(hi.second, p.second);
    }
}
