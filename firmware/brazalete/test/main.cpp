// Corre todas las pruebas registradas con PRUEBA(...) y devuelve 1 si alguna falla.
#include "minitest.h"

int main() {
    int fallidas = 0;
    for (const minitest::Prueba& prueba : minitest::registro()) {
        minitest::fallos = 0;
        prueba.funcion();
        std::printf("[%s] %s\n", minitest::fallos == 0 ? " OK " : "FALLA", prueba.nombre);
        if (minitest::fallos != 0) ++fallidas;
    }
    std::printf("\n%d pruebas, %d fallidas\n", (int)minitest::registro().size(), fallidas);
    return fallidas == 0 ? 0 : 1;
}
