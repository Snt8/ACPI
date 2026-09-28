#ifndef COMUNICACION_MOVIL_H
#define COMUNICACION_MOVIL_H

#include <Arduino.h>
#include <stdint.h>

class ComunicadorMovil {
public:
    static bool inicializar();
    static bool estaConectado();
    static void enviarDatos(uint16_t heading, uint8_t estado, bool orientacionOk, uint8_t tiempoRestante, int8_t bateria);
    static void desinicializar();
};

#endif // COMUNICACION_MOVIL_H
