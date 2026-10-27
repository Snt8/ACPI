#include "motor.h"
#include <stdint.h>
#include <Arduino.h>
#include "../constantes.h"

// ── Capa de compatibilidad LEDC (core v2.x / v3.x) ───────────────────────────
static bool configurarPWM(uint8_t pin, uint8_t canal, uint16_t frecuencia, uint8_t resolucion) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    (void)canal;
    return ledcAttach(pin, frecuencia, resolucion);
#else
    (void)pin;
    ledcSetup(canal, frecuencia, resolucion);
    ledcAttachPin(pin, canal);
    return true;
#endif
}

static void escribirPWM(uint8_t pin, uint8_t canal, uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    (void)canal;
    ledcWrite(pin, duty);
#else
    (void)pin;
    ledcWrite(canal, duty);
#endif
}

// Estado conocido de cada motor, para no reescribir el PWM en cada vuelta del loop
bool ControladorMotores::encendido_derecho   = false;
bool ControladorMotores::encendido_izquierdo = false;

bool ControladorMotores::inicializar() {
    bool ok = configurarPWM(pin_motor_derecho,   canal_motor_derecho,   frecuencia, resolucion);
    ok     &= configurarPWM(pin_motor_izquierdo, canal_motor_izquierdo, frecuencia, resolucion);

    // Nos aseguramos de que los motores arranquen apagados
    detenerMotor(AMBOS);

    return ok;
}

void ControladorMotores::aplicar(bool derecho, bool izquierdo) {
    if (derecho != encendido_derecho) {
        escribirPWM(pin_motor_derecho, canal_motor_derecho,
                    derecho ? INTENSIDAD_MOTOR : APAGAR_MOTOR);
        encendido_derecho = derecho;
    }
    if (izquierdo != encendido_izquierdo) {
        escribirPWM(pin_motor_izquierdo, canal_motor_izquierdo,
                    izquierdo ? INTENSIDAD_MOTOR_IZQUIERDO : APAGAR_MOTOR);
        encendido_izquierdo = izquierdo;
    }
}

void ControladorMotores::vibrarMotor(Motor motor) {
    switch (motor) {
        case DERECHO:   aplicar(true, encendido_izquierdo); break;
        case IZQUIERDO: aplicar(encendido_derecho, true);   break;
        case AMBOS:     aplicar(true, true);                break;
    }
}

void ControladorMotores::detenerMotor(Motor motor) {
    switch (motor) {
        case DERECHO:   aplicar(false, encendido_izquierdo); break;
        case IZQUIERDO: aplicar(encendido_derecho, false);   break;
        case AMBOS:
            // Se escribe siempre (no sólo al cambiar): al arrancar, el estado
            // guardado puede no coincidir con lo que tiene el pin
            escribirPWM(pin_motor_derecho,   canal_motor_derecho,   APAGAR_MOTOR);
            escribirPWM(pin_motor_izquierdo, canal_motor_izquierdo, APAGAR_MOTOR);
            encendido_derecho = encendido_izquierdo = false;
            break;
    }
}
