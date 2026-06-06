#ifndef CONTROLADOR_BRUJULA_SEMAFORO_H
#define CONTROLADOR_BRUJULA_SEMAFORO_H

#include <stdint.h>

class ControladorBrujulaSemaforo {
private:
    static float x;
    static float y;
    static float z;

public:
    static float headingActual;
    static bool inicializar();
    static bool leerHeading();
};

#endif // CONTROLADOR_BRUJULA_SEMAFORO_H
