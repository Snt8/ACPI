#ifndef MINITEST_H
#define MINITEST_H
// Mini framework de pruebas, sin dependencias: cada PRUEBA se registra sola y
// main.cpp las corre todas. Compila con MSVC, GCC y Clang (C++17).

#include <cstdio>
#include <vector>

namespace minitest {

struct Prueba {
    const char* nombre;
    void (*funcion)();
};

inline std::vector<Prueba>& registro() {
    static std::vector<Prueba> pruebas;
    return pruebas;
}

inline int fallos = 0;  // fallos de la prueba en curso

struct Registrar {
    Registrar(const char* nombre, void (*funcion)()) { registro().push_back({nombre, funcion}); }
};

} // namespace minitest

#define PRUEBA(nombre)                                                    \
    static void nombre();                                                 \
    static minitest::Registrar registrar_##nombre(#nombre, nombre);       \
    static void nombre()

#define VERIFICAR(condicion)                                              \
    do {                                                                  \
        if (!(condicion)) {                                               \
            std::printf("    FALLO %s:%d: %s\n", __FILE__, __LINE__, #condicion); \
            ++minitest::fallos;                                           \
        }                                                                 \
    } while (0)

// Compara como enteros largos: sirve para números, bool y enum class
#define VERIFICAR_IGUAL(obtenido, esperado)                               \
    do {                                                                  \
        long long o_ = (long long)(obtenido), e_ = (long long)(esperado); \
        if (o_ != e_) {                                                   \
            std::printf("    FALLO %s:%d: %s == %lld, se esperaba %s == %lld\n", \
                        __FILE__, __LINE__, #obtenido, o_, #esperado, e_); \
            ++minitest::fallos;                                           \
        }                                                                 \
    } while (0)

#define VERIFICAR_CERCA(obtenido, esperado, tolerancia)                   \
    do {                                                                  \
        double o_ = (double)(obtenido), e_ = (double)(esperado);          \
        if (o_ - e_ > (tolerancia) || e_ - o_ > (tolerancia)) {           \
            std::printf("    FALLO %s:%d: %s == %.4f, se esperaba %.4f\n", \
                        __FILE__, __LINE__, #obtenido, o_, e_);           \
            ++minitest::fallos;                                           \
        }                                                                 \
    } while (0)

#endif // MINITEST_H
