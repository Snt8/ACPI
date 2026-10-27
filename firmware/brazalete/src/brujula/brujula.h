#ifndef BRUJULA_H
#define BRUJULA_H

#include <stdint.h>
#include <Arduino.h>

// ── Controlador de brújula (fusión MPU6050 + QMC5883L) ─────────────────────────
// Encapsula la lectura de ambos sensores, la tilt compensation, la calibración
// hard-iron y el safety check de cambio rápido de heading.
class ControladorBrujula {
private:
    // Estado del safety check
    static float    headingAnterior;
    static unsigned long tiempoUltimaLectura;
    static bool     lecturaValida;

public:
    // Heading actual en grados [0, 360)
    static float headingActual;

    // Inicializa MPU6050 y QMC5883L
    static bool inicializar();

    // Lee sensores, aplica tilt compensation y actualiza headingActual.
    // Internamente evalúa el safety check: si el heading cambia > UMBRAL_DELTA_HEADING
    // en menos de UMBRAL_TIEMPO_SAFETY_MS, invalida la lectura.
    static void obtenerHeading();

    // Retorna true si la última lectura de heading fue considerada válida
    // (no fue rechazada por el safety check).
    static bool headingValido();

    // Para saber si la persona está de frente al cruce se usa
    // desviacionEnfrentamiento() de utils/calcular_direccion.h, y la decisión
    // (con histéresis) la toma EvaluadorSemaforo.
};

#endif // BRUJULA_H