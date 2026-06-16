// Experimentos de la Entrega 3 (version C++).
// Corre NSGA-II con 10 semillas sobre los 3 casos, construye el frente agregado
// y compara contra el frente exacto (solver) mediante hypervolume y two-set
// coverage. Genera CSVs en resultados/.
//
// Uso:  ./nsga2  [--rapido]

#include "nsga2.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <string>
#include <cstring>

struct Caso { std::string nombre, archivo; };

static void guarda_frente(const std::string& ruta, std::vector<Punto> fr) {
    std::sort(fr.begin(), fr.end());
    std::ofstream o(ruta);
    o << "f1_costo,f2_desbalance\n";
    o << std::fixed << std::setprecision(4);
    for (auto& p : fr) o << p.first << "," << p.second << "\n";
}

int main(int argc, char** argv) {
    bool rapido = (argc > 1 && std::strcmp(argv[1], "--rapido") == 0);
    int n_sem = rapido ? 3 : 10;
    int popsize = rapido ? 80 : 100;
    int ngen = rapido ? 120 : 250;
    double pcross = 0.9, pmut = 0.3;

    std::vector<Caso> casos = {
        {"A", "data/Instance_p30_d3_d1_p0.004_s129.dat"},
        {"B", "data/Instance_p30_d3_d1_p0.204_s762.dat"},
        {"C", "data/Instance_p30_d3_d1_p0.204_s762_3camiones.dat"},
    };
    std::map<std::string,std::vector<Punto>> solver = {
        {"A", {{52900.75,162.0}}},
        {"B", {{78592.13,1843.0},{75341.31,1856.0},{75047.94,1871.0},{73127.06,2077.0}}},
        {"C", {{108221.65,1.0},{96690.81,44.0},{75047.94,1871.0},{73127.06,2077.0}}},
    };

    std::ofstream resumen("resultados/resumen_indicadores.csv");
    resumen << "caso,n_semillas,pts_agregado,hv_solver,hv_nsga,C_solver_nsga,C_nsga_solver,tiempo_s\n";

    std::cout << std::fixed << std::setprecision(2);
    for (auto& caso : casos) {
        Instancia inst;
        if (!inst.cargar(caso.archivo)) {
            std::cerr << "No pude leer " << caso.archivo << "\n"; return 1;
        }
        std::cout << "\n===== CASO " << caso.nombre << "  (" << caso.archivo << ") =====\n";
        std::cout << inst.resumen() << "\n";

        std::vector<std::vector<Punto>> frentes_sem;
        auto t0 = std::chrono::steady_clock::now();
        for (int s = 1; s <= n_sem; ++s) {
            RNG rng((unsigned)s);
            auto P = nsga2(inst, rng, popsize, ngen, pcross, pmut);
            auto fr = filtra_no_dominados(frente_factible(P));
            frentes_sem.push_back(fr);
            guarda_frente("resultados/frente_" + caso.nombre + "_semilla" +
                          (s < 10 ? "0" : "") + std::to_string(s) + ".csv", fr);
            double mc = 1e18, mb = 1e18;
            for (auto& p : fr) { mc = std::min(mc, p.first); mb = std::min(mb, p.second); }
            std::cout << "  semilla " << std::setw(2) << s << " -> " << std::setw(2)
                      << fr.size() << " puntos, mejor costo=" << mc
                      << ", mejor balance=" << mb << "\n";
        }
        auto t1 = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(t1 - t0).count();

        auto agregado = frente_agregado(frentes_sem);
        guarda_frente("resultados/frente_" + caso.nombre + "_agregado.csv", agregado);

        Punto lo, hi;
        rango_comun(solver[caso.nombre], agregado, lo, hi);
        Punto ref = {hi.first + 0.1 * (hi.first - lo.first + 1),
                     hi.second + 0.1 * (hi.second - lo.second + 1)};
        double hv_s = hypervolume_2d(solver[caso.nombre], ref, lo, hi);
        double hv_n = hypervolume_2d(agregado, ref, lo, hi);
        double c_sn = two_set_coverage(solver[caso.nombre], agregado);
        double c_ns = two_set_coverage(agregado, solver[caso.nombre]);

        std::cout << "  --- indicadores ---\n";
        std::cout << "  HV solver=" << std::setprecision(4) << hv_s
                  << "   HV NSGA-II=" << hv_n << std::setprecision(2) << "\n";
        std::cout << "  C(solver,NSGA)=" << c_sn << "   C(NSGA,solver)=" << c_ns << "\n";
        std::cout << "  tiempo total caso=" << dt << "s\n";

        resumen << caso.nombre << "," << n_sem << "," << agregado.size() << ","
                << std::setprecision(4) << hv_s << "," << hv_n << ","
                << std::setprecision(2) << c_sn << "," << c_ns << "," << dt << "\n";
    }
    std::cout << "\nResultados en: resultados/\n";
    return 0;
}
