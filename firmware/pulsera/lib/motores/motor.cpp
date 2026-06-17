#include "motor.h"
#include <stdint.h>
#include <Arduino.h>
#include "constantes.h"

bool ControladorMotores::inicializar() {
    // Reemplazo para el SDK actual: configurar canal + asociar pin
    // canal_motor_* definidos en motor.h
    ledcSetup(canal_motor_derecho,   frecuencia, resolucion);
    ledcAttachPin(pin_motor_derecho,   canal_motor_derecho);
    ledcSetup(canal_motor_izquierdo, frecuencia, resolucion);
    ledcAttachPin(pin_motor_izquierdo, canal_motor_izquierdo);

    // Nos aseguramos de que los motores arranquen apagados
    // Ahora ledcWrite espera el canal, no el pin
    ledcWrite(canal_motor_derecho,   APAGAR_MOTOR);
    ledcWrite(canal_motor_izquierdo, APAGAR_MOTOR);

    return true;
}

void ControladorMotores::vibrarMotor(Motor motor, uint8_t intensidad) {
    switch(motor) {
        case DERECHO:   ledcWrite(canal_motor_derecho,   intensidad); break;
        case IZQUIERDO: ledcWrite(canal_motor_izquierdo, INTENSIDAD_MOTOR_IZQUIERDO); break;
        case AMBOS:
            ledcWrite(canal_motor_derecho,   intensidad);
            ledcWrite(canal_motor_izquierdo, INTENSIDAD_MOTOR_IZQUIERDO);
            break;
    }
}

void ControladorMotores::detenerMotor(Motor motor) {
    switch(motor) {
        case DERECHO:   ledcWrite(canal_motor_derecho,   APAGAR_MOTOR); break;
        case IZQUIERDO: ledcWrite(canal_motor_izquierdo, APAGAR_MOTOR); break;
        case AMBOS:
            ledcWrite(canal_motor_derecho,   APAGAR_MOTOR);
            ledcWrite(canal_motor_izquierdo, APAGAR_MOTOR);
            break;
    }
}