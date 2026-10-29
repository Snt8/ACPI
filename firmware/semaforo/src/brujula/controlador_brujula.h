#ifndef CONTROLADOR_BRUJULA_SEMAFORO_H
#define CONTROLADOR_BRUJULA_SEMAFORO_H

#include <stdint.h>

// ── Controlador de brújula para el Semáforo (QMC6308, I2C 0x2C) ───────────────
// El semáforo está fijo en posición horizontal, por lo que NO aplica
// tilt compensation. El heading se calcula directamente con atan2(y, x).
//
// Calibración hard-iron (ver calibracion_magnetica.h): se mide una vez
// girando el semáforo en el plano, ANTES de instalarlo, y queda guardada en la
// memoria no volátil. Como el semáforo no se mueve, el rumbo enviado es el
// promedio circular de las últimas LECTURAS_PROMEDIO lecturas.
class ControladorBrujulaSemaforo {
public:
    // Rumbo promediado en grados [0, 360)
    static float headingActual;

    // Inicializa el QMC6308 (escáner I2C, soft reset, modo continuo) y carga
    // la calibración guardada, si la hay
    static bool inicializar();

    // Lee 6 bytes del sensor, aplica la calibración y actualiza el promedio
    static bool leerHeading();

    // Grados para el paquete: headingActual redondeado, 0-359
    static uint16_t gradosParaEnviar();

    // ── Calibración ──────────────────────────────────────────────────────────
    static bool calibrada();
    // Bloquea hasta que la calibración esté lista y estable o pasen
    // duracion_max_ms. Hay que girar el semáforo, plano, al menos una vuelta
    // completa. Si termina bien la guarda.
    static bool calibrar(unsigned long duracion_max_ms);
    static void borrarCalibracion();
};

#endif // CONTROLADOR_BRUJULA_SEMAFORO_H
