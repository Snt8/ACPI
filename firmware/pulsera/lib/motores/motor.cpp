#include "motor.h"
#include <stdint.h>
#include <Arduino.h>
#include "constantes.h"

bool ControladorMotores::inicializar() {
    ledcSetup(canal_motor_derecho,   frecuencia, resolucion);
    ledcSetup(canal_motor_izquierdo, frecuencia, resolucion);

    ledcAttachPin(pin_motor_derecho,   canal_motor_derecho);
    ledcAttachPin(pin_motor_izquierdo, canal_motor_izquierdo);

    ledcWrite(canal_motor_derecho,   0);
    ledcWrite(canal_motor_izquierdo, 0);

    return true;
}

void ControladorMotores::vibrarMotor(Motor motor, uint8_t intensidad) {
    switch(motor) {
        case DERECHO:   ledcWrite(canal_motor_derecho,   intensidad); break;
        case IZQUIERDO: ledcWrite(canal_motor_izquierdo, intensidad); break;
        case AMBOS:
            ledcWrite(canal_motor_derecho,   intensidad);
            ledcWrite(canal_motor_izquierdo, intensidad);
            break;
    }
}

void ControladorMotores::detenerMotor(Motor motor) {
    switch(motor) {
        case DERECHO:   ledcWrite(canal_motor_derecho,   0); break;
        case IZQUIERDO: ledcWrite(canal_motor_izquierdo, 0); break;
        case AMBOS:
            ledcWrite(canal_motor_derecho,   0);
            ledcWrite(canal_motor_izquierdo, 0);
            break;
    }
}