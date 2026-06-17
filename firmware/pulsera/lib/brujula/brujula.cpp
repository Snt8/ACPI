#include "brujula.h"
#include "../sensores/mpu6050.h"
#include "../sensores/qmc5883l.h"
#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "constantes.h"

// ── Variables estáticas ───────────────────────────────────────────────────────
float        ControladorBrujula::headingActual      = 0.0f;
float        ControladorBrujula::headingAnterior    = 0.0f;
unsigned long ControladorBrujula::tiempoUltimaLectura = 0;
bool          ControladorBrujula::lecturaValida     = false;

// ── Inicialización ────────────────────────────────────────────────────────────
bool ControladorBrujula::inicializar() {
    bool acelerometro_ok = ControladorAcelerometro::inicializar();
    bool magnetometro_ok = ControladorMagnetometro::inicializar();

    if (acelerometro_ok && magnetometro_ok) {
        tiempoUltimaLectura = millis();
        return true;
    }
    return false;
}

// ── Lectura de sensores con tilt compensation y safety check ──────────────────
void ControladorBrujula::obtenerHeading() {
    unsigned long ahora = millis();

    // 1. Leer acelerómetro y obtener valores normalizados en g
    bool accel_ok = ControladorAcelerometro::leerDatos();
    float gx = ControladorAcelerometro::getGx();
    float gy = ControladorAcelerometro::getGy();
    float gz = ControladorAcelerometro::getGz();

    // 2. Leer magnetómetro (incluye calibración hard-iron)
    bool mag_ok = ControladorMagnetometro::leerPosicion();

    // Si cualquier sensor falló, invalidar lectura inmediatamente — no continuar
    if (!accel_ok || !mag_ok) {
        lecturaValida = false;
        tiempoUltimaLectura = ahora;  // Resetear referencia para evitar bypass del safety check
        return;
    }

    // 3. Calcular heading con tilt compensation usando gx, gy, gz directamente
    float nuevo_heading = ControladorMagnetometro::calcularHeading(gx, gy, gz);

    // ── Safety check ──────────────────────────────────────────────────────────
    // Si el heading cambia más de UMBRAL_DELTA_HEADING grados en menos de
    // UMBRAL_TIEMPO_SAFETY_MS milisegundos, se considera una lectura inválida
    // (movimiento brusco o ruido). No se actualiza headingActual.
    unsigned long dt = ahora - tiempoUltimaLectura;

    if (dt < UMBRAL_TIEMPO_SAFETY_MS) {
        // Calcular diferencia angular mínima en el círculo
        float delta = fabsf(nuevo_heading - headingAnterior);
        if (delta > GRADOS_CIRCUNFERENCIA / 2.0f) {
            delta = GRADOS_CIRCUNFERENCIA - delta;
        }

        if (delta > UMBRAL_DELTA_HEADING) {
            // Lectura rechazada: cambio de heading demasiado rápido
            lecturaValida = false;
            Serial.printf("[BRUJULA] Safety: delta=%.1f deg en %lums — lectura invalida\n",
                          delta, dt);
            tiempoUltimaLectura = ahora;
            headingAnterior = nuevo_heading;  // actualizar referencia para el próximo ciclo
            return;
        }
    }

    // Lectura válida: actualizar estado
    headingActual       = nuevo_heading;
    headingAnterior     = nuevo_heading;
    tiempoUltimaLectura = ahora;
    lecturaValida       = true;
}

// ── Estado de validez de la última lectura ────────────────────────────────────
bool ControladorBrujula::headingValido() {
    return lecturaValida;
}

// ── Comparación diferencial: condición de cruce seguro ───────────────────────
// La condición correcta para que el usuario esté orientado para cruzar es que
// los dos nodos se ENFRENTEN, es decir, su diferencia angular sea ≈180°.
//
// Δθ = min(|Hp - Hs|, 360 - |Hp - Hs|)   → diferencia angular mínima [0, 180]
// Condición de cruce seguro: |Δθ - 180°| < UMBRAL_CRUCE_GRADOS
//
// IMPORTANTE: No es Δθ < 30° (que significaría que apuntan en la misma dirección),
// sino |Δθ - 180°| < 30° (que significa que están enfrentados).
bool ControladorBrujula::revisarSemaforo(uint16_t posicionSemaforo) {
    float Hp = headingActual;
    float Hs = (float)posicionSemaforo;

    // Diferencia angular absoluta
    float diff = fabsf(Hp - Hs);

    // Normalizar al rango [0, 180]: tomar el arco menor
    float delta_theta = (diff > 180.0f) ? (GRADOS_CIRCUNFERENCIA - diff) : diff;

    // Condición de enfrentamiento: Δθ ≈ 180°
    float desviacion = fabsf(delta_theta - 180.0f);

    return (desviacion < UMBRAL_CRUCE_GRADOS);
}
