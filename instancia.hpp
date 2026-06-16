// Lectura de instancias .dat (formato AMPL).
#ifndef INSTANCIA_HPP
#define INSTANCIA_HPP

#include <string>
#include <vector>
#include <map>
#include <utility>

struct Instancia {
    std::vector<int> M, N, K, R;                 // depositos, puntos, camiones, residuos
    std::vector<std::pair<int,int>> vehicle_map; // (deposito, camion) ordenado -> indice de vehiculo
    int n_vehiculos = 0;
    std::map<int,int> deposito_de_camion;        // camion -> deposito base

    std::map<int,double> latitud, longitud;      // solo para graficar
    std::map<int,double> F, alfa;                // costo fijo y variable por camion
    std::map<std::pair<int,int>,double> Q;       // (r,k) -> capacidad compartimento
    std::map<std::pair<int,int>,double> q;       // (i,r) -> cantidad de residuo
    std::map<std::pair<int,int>,double> D;       // (i,j) -> distancia
    std::map<int,double> carga_punto;            // i -> suma sobre r de q[i,r]

    // parametros de turno/servicio (defaults de casoP.mod)
    double gamma = 1.0, eta = 1.0, beta = 0.0, Tmax = 999999.0;

    bool cargar(const std::string& ruta);
    double dist(int a, int b) const;
    std::vector<int> depositos_activos() const;
    std::string resumen() const;
};

#endif
