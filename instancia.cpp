#include "instancia.hpp"
#include <fstream>
#include <sstream>
#include <set>
#include <cctype>
#include <cmath>

// ----- utilidades de parseo -----
static std::string leer_archivo(const std::string& ruta) {
    std::ifstream fh(ruta);
    std::stringstream ss;
    ss << fh.rdbuf();
    return ss.str();
}

static std::vector<std::string> tokens(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string w;
    while (ss >> w) out.push_back(w);
    return out;
}

static bool es_entero(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (i >= s.size()) return false;
    for (; i < s.size(); ++i)
        if (!std::isdigit((unsigned char)s[i])) return false;
    return true;
}

// contenido entre el ":=" que sigue a 'clave' y el ';' siguiente
static std::string bloque_asignacion(const std::string& t, const std::string& clave) {
    size_t p = t.find(clave);
    if (p == std::string::npos) return "";
    size_t a = t.find(":=", p);
    if (a == std::string::npos) return "";
    size_t e = t.find(';', a);
    return t.substr(a + 2, e - (a + 2));
}

static std::map<int,double> pares_num(const std::string& bloque) {
    std::map<int,double> d;
    auto tk = tokens(bloque);
    for (size_t i = 0; i + 1 < tk.size(); i += 2)
        d[std::stoi(tk[i])] = std::stod(tk[i + 1]);
    return d;
}

static std::map<std::pair<int,int>,double> triples(const std::string& bloque) {
    std::map<std::pair<int,int>,double> d;
    auto tk = tokens(bloque);
    for (size_t i = 0; i + 2 < tk.size(); i += 3)
        d[{std::stoi(tk[i]), std::stoi(tk[i + 1])}] = std::stod(tk[i + 2]);
    return d;
}

static std::vector<int> set_enteros(const std::string& t, const std::string& nombre) {
    std::string clave = "set " + nombre;
    std::string b = bloque_asignacion(t, clave);
    std::vector<int> out;
    for (auto& w : tokens(b))
        if (es_entero(w)) out.push_back(std::stoi(w));
    return out;
}

bool Instancia::cargar(const std::string& ruta) {
    std::string t = leer_archivo(ruta);
    if (t.empty()) return false;

    M = set_enteros(t, "M");
    N = set_enteros(t, "N");
    K = set_enteros(t, "K");
    R = set_enteros(t, "R");

    // set MK := ( dep , k ) ...
    {
        std::string b = bloque_asignacion(t, "set MK");
        std::vector<int> nums;
        for (auto& w : tokens(b))
            if (es_entero(w)) nums.push_back(std::stoi(w));
        for (size_t i = 0; i + 1 < nums.size(); i += 2) {
            int dep = nums[i], k = nums[i + 1];
            vehicle_map.push_back({dep, k});
            deposito_de_camion[k] = dep;
        }
        n_vehiculos = (int)vehicle_map.size();
    }

    latitud  = pares_num(bloque_asignacion(t, "latitud"));
    longitud = pares_num(bloque_asignacion(t, "longitud"));
    F        = pares_num(bloque_asignacion(t, "param F"));
    alfa     = pares_num(bloque_asignacion(t, "param alfa"));
    Q        = triples(bloque_asignacion(t, "param Q"));
    q        = triples(bloque_asignacion(t, "param q"));

    // matriz D: "param D :  col col ... :=  fila v v ... ;"
    {
        size_t p = t.find("param D");
        size_t colon = t.find(':', p);
        size_t assign = t.find(":=", p);
        std::string cols_s = t.substr(colon + 1, assign - (colon + 1));
        std::string body = t.substr(assign + 2, t.find(';', assign) - (assign + 2));
        std::vector<int> cols;
        for (auto& w : tokens(cols_s))
            if (es_entero(w)) cols.push_back(std::stoi(w));
        auto tk = tokens(body);
        size_t i = 0;
        while (i < tk.size()) {
            int fila = std::stoi(tk[i++]);
            for (size_t c = 0; c < cols.size() && i < tk.size(); ++c)
                D[{fila, cols[c]}] = std::stod(tk[i++]);
        }
    }

    // carga total por punto = suma sobre tipos de residuo
    for (int i : N) {
        double s = 0.0;
        for (int r : R) {
            auto it = q.find({i, r});
            if (it != q.end()) s += it->second;
        }
        carga_punto[i] = s;
    }
    return true;
}

double Instancia::dist(int a, int b) const {
    if (a == b) return 0.0;
    auto it = D.find({a, b});
    return it == D.end() ? 0.0 : it->second;
}

std::vector<int> Instancia::depositos_activos() const {
    std::set<int> s;
    for (auto& vm : vehicle_map) s.insert(vm.first);
    return std::vector<int>(s.begin(), s.end());
}

std::string Instancia::resumen() const {
    std::stringstream ss;
    double tot = 0.0;
    for (auto& kv : carga_punto) tot += kv.second;
    ss << "  |N|=" << N.size() << " puntos, |M|=" << M.size()
       << " depositos, |K|=" << K.size() << " camiones, |R|=" << R.size()
       << " residuos\n  vehicle_map=";
    for (auto& vm : vehicle_map) ss << "(" << vm.first << "," << vm.second << ") ";
    ss << "\n  demanda total=" << (long)(tot + 0.5);
    return ss.str();
}
