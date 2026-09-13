// Adaptacion de NSGA-II al problema de recoleccion (representacion entera de
// rutas con separadores). Declaraciones y utilidades compartidas.
#ifndef NSGA2_HPP
#define NSGA2_HPP

#include "instancia.hpp"
#include <vector>
#include <random>
#include <utility>
#include <algorithm>

// ------------------------- Generador aleatorio -------------------------
struct RNG {
    std::mt19937 gen;
    explicit RNG(unsigned s) : gen(s) {}
    int randint(int a, int b) {                 // entero en [a,b]
        return std::uniform_int_distribution<int>(a, b)(gen);
    }
    int randrange(int n) {                       // entero en [0,n-1]
        return std::uniform_int_distribution<int>(0, n - 1)(gen);
    }
    double rand01() {
        return std::uniform_real_distribution<double>(0.0, 1.0)(gen);
    }
    template <class T> const T& choice(const std::vector<T>& v) {
        return v[randrange((int)v.size())];
    }
    template <class T> void shuffle(std::vector<T>& v) {
        for (int i = (int)v.size() - 1; i > 0; --i)
            std::swap(v[i], v[randint(0, i)]);
    }
    std::pair<int,int> sample2(int n) {          // dos indices distintos en [0,n-1]
        int a = randrange(n);
        int b = randrange(n - 1);
        if (b >= a) ++b;
        return {a, b};
    }
};

// ------------------------------ Individuo ------------------------------
struct Individuo {
    std::vector<int> crom;
    double f1 = 0, f2 = 0, viol = 0;
    bool factible = false;
    int rank = 0;
    double cd = 0.0;
};

typedef std::vector<std::vector<int>> Rutas;

// --------------------------- Representacion ----------------------------
Rutas decode(const std::vector<int>& crom, const Instancia& inst);
std::vector<int> codifica(const Rutas& rutas, const Instancia& inst);
bool es_valido(const std::vector<int>& crom, const Instancia& inst);
std::vector<int> repara(const std::vector<int>& crom, const Instancia& inst, RNG& rng);
std::vector<int> individuo_aleatorio(const Instancia& inst, RNG& rng);

// ------------------------------ Evaluacion -----------------------------
void evalua(Individuo& ind, const Instancia& inst);

// ----------------------------- Inicializacion --------------------------
std::vector<int> greedy_cercania(const Instancia& inst, RNG& rng);
std::vector<int> orientada_balance(const Instancia& inst, RNG& rng);
std::vector<std::vector<int>> inicializa_poblacion(const Instancia& inst,
                                                   int popsize, RNG& rng);

// ------------------------------ Operadores -----------------------------
std::vector<int> cruzamiento(const std::vector<int>& a, const std::vector<int>& b,
                             const Instancia& inst, RNG& rng);
std::vector<int> mutacion(const std::vector<int>& crom, const Instancia& inst, RNG& rng);

// --------------------------- Nucleo NSGA-II ----------------------------
std::vector<Individuo> nsga2(const Instancia& inst, RNG& rng,
                             int popsize, int ngen,
                             double pcross, double pmut);
std::vector<std::pair<double,double>> frente_factible(const std::vector<Individuo>& pop);

// ------------------------------ Indicadores ----------------------------
typedef std::pair<double,double> Punto;
std::vector<Punto> filtra_no_dominados(const std::vector<Punto>& pts);
std::vector<Punto> frente_agregado(const std::vector<std::vector<Punto>>& frentes);
double hypervolume_2d(const std::vector<Punto>& frente, Punto ref, Punto lo, Punto hi);
double two_set_coverage(const std::vector<Punto>& A, const std::vector<Punto>& B);
void rango_comun(const std::vector<Punto>& a, const std::vector<Punto>& b,
                 Punto& lo, Punto& hi);

#endif
