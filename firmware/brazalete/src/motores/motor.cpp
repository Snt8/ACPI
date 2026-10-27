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

bool ControladorMotores::inicializar() {
    bool ok = configurarPWM(pin_motor_derecho,   canal_motor_derecho,   frecuencia, resolucion);
    ok     &= configurarPWM(pin_motor_izquierdo, canal_motor_izquierdo, frecuencia, resolucion);

    // Nos aseguramos de que los motores arranquen apagados
    detenerMotor(AMBOS);

    return ok;
}

void ControladorMotores::vibrarMotor(Motor motor, uint8_t intensidad) {
    switch(motor) {
        case DERECHO:   escribirPWM(pin_motor_derecho,   canal_motor_derecho,   intensidad); break;
        case IZQUIERDO: escribirPWM(pin_motor_izquierdo, canal_motor_izquierdo, INTENSIDAD_MOTOR_IZQUIERDO); break;
        case AMBOS:
            escribirPWM(pin_motor_derecho,   canal_motor_derecho,   intensidad);
            escribirPWM(pin_motor_izquierdo, canal_motor_izquierdo, INTENSIDAD_MOTOR_IZQUIERDO);
            break;
    }
}

void ControladorMotores::detenerMotor(Motor motor) {
    switch(motor) {
        case DERECHO:   escribirPWM(pin_motor_derecho,   canal_motor_derecho,   APAGAR_MOTOR); break;
        case IZQUIERDO: escribirPWM(pin_motor_izquierdo, canal_motor_izquierdo, APAGAR_MOTOR); break;
        case AMBOS:
            escribirPWM(pin_motor_derecho,   canal_motor_derecho,   APAGAR_MOTOR);
            escribirPWM(pin_motor_izquierdo, canal_motor_izquierdo, APAGAR_MOTOR);
            break;
    }
}
