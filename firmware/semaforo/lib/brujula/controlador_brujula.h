#ifndef CONTROLADOR_BRUJULA_SEMAFORO_H
#define CONTROLADOR_BRUJULA_SEMAFORO_H

#include <stdint.h>

// ── Controlador de brújula para el Semáforo (QMC6308, I2C 0x2C) ───────────────
// El semáforo está fijo en posición horizontal, por lo que NO aplica
// tilt compensation. El heading se calcula directamente con atan2(y, x).
//
// Incluye calibración hard-iron mediante Leaky Integrator para compensar
// el campo magnético estático del entorno urbano.
class ControladorBrujulaSemaforo {
private:
    static float x;         // Lectura magnética corregida eje X [LSB]
    static float y;         // Lectura magnética corregida eje Y [LSB]
    static float z;         // Lectura magnética corregida eje Z [LSB]

    // Offsets de calibración hard-iron (Leaky Integrator)
    static float offset_x;
    static float offset_y;
    static float offset_z;

public:
    static float headingActual;

    // Inicializa el QMC6308: escáner I2C, soft reset y modo continuo
    static bool inicializar();

    // Lee 6 bytes del sensor, aplica calibración hard-iron y calcula el heading
    static bool leerHeading();
};

#endif // CONTROLADOR_BRUJULA_SEMAFORO_H
