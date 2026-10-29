#ifndef BRUJULA_H
#define BRUJULA_H

#include <stdint.h>
#include <Arduino.h>

// ── Controlador de brújula (fusión MPU6500 + QMC5883L) ─────────────────────────
// Encapsula la lectura de ambos sensores, la calibración del magnetómetro, la
// tilt compensation y el safety check de cambio rápido de heading.
//
// Calibración: el campo fijo del propio brazalete (sobre todo los imanes de
// los motores) desvía el rumbo hasta ~40°. Se mide UNA vez girando el
// brazalete en todas direcciones y queda guardada en la memoria no volátil
// (sobrevive a reinicios y a cargar un programa nuevo). Sin calibración el
// rumbo no es fiable: headingValido() devuelve false y el brazalete no indica
// giros ni avisa la luz (sólo "semáforo detectado").
class ControladorBrujula {
private:
    // Estado del safety check
    static float    headingAnterior;
    static unsigned long tiempoUltimaLectura;
    static bool     lecturaValida;

public:
    // Heading actual en grados [0, 360)
    static float headingActual;

    // Inicializa MPU6500 y QMC5883L. La primera vez carga la calibración
    // guardada, si la hay.
    static bool inicializar();

    // Standby del brazalete: duerme ambos sensores para que no consuman
    // mientras nadie los lee. despertar() los vuelve a configurar.
    static void dormir();
    static bool despertar();

    // Lee sensores, aplica calibración y tilt compensation y actualiza
    // headingActual. Internamente evalúa el safety check: si el heading cambia
    // > UMBRAL_DELTA_HEADING respecto a una lectura de hace menos de
    // UMBRAL_TIEMPO_SAFETY_MS (la muestra anterior), invalida la lectura.
    static void obtenerHeading();

    // Retorna true si la última lectura de heading fue considerada válida
    // (brújula calibrada, sensores bien, muñeca quieta y sin giro brusco).
    static bool headingValido();

    // ── Calibración ──────────────────────────────────────────────────────────
    static bool calibrada();
    // Bloquea hasta que la calibración esté lista y estable o pasen
    // CALIBRACION_DURACION_MAX_MS. Mientras tanto hay que girar el brazalete
    // en todas direcciones (también boca abajo), como dibujando un 8. Si
    // termina bien la guarda. Los motores deben estar apagados.
    static bool calibrar();
    // Borra la calibración guardada (la siguiente vez que arranque, calibra)
    static void borrarCalibracion();

    // Para saber si la persona está de frente al cruce se usa
    // desviacionEnfrentamiento() de utils/calcular_direccion.h, y la decisión
    // (con histéresis) la toma EvaluadorSemaforo.
};

#endif // BRUJULA_H
