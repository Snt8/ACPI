#include "motor.h"
#include <stdint.h>
#include <Arduino.h>
#include "constantes.h"

bool ControladorMotores::inicializar() {
    // API nueva de espressif32@6.9.0 (Arduino ESP32 v3.x):
    // ledcAttach(pin, frecuencia, resolucion) reemplaza a ledcSetup + ledcAttachPin
    ledcAttach(pin_motor_derecho,   frecuencia, resolucion);
    ledcAttach(pin_motor_izquierdo, frecuencia, resolucion);

    // Nos aseguramos de que los motores arranquen apagados
    ledcWrite(pin_motor_derecho,   APAGAR_MOTOR);
    ledcWrite(pin_motor_izquierdo, APAGAR_MOTOR);

    return true;
}

void ControladorMotores::vibrarMotor(Motor motor, uint8_t intensidad) {
    switch(motor) {
        case DERECHO:   ledcWrite(pin_motor_derecho,   intensidad); break;
        case IZQUIERDO: ledcWrite(pin_motor_izquierdo, intensidad); break;
        case AMBOS:
            ledcWrite(pin_motor_derecho,   intensidad);
            ledcWrite(pin_motor_izquierdo, intensidad);
            break;
    }
}

void ControladorMotores::detenerMotor(Motor motor) {
    switch(motor) {
        case DERECHO:   ledcWrite(pin_motor_derecho,   APAGAR_MOTOR); break;
        case IZQUIERDO: ledcWrite(pin_motor_izquierdo, APAGAR_MOTOR); break;
        case AMBOS:
            ledcWrite(pin_motor_derecho,   APAGAR_MOTOR);
            ledcWrite(pin_motor_izquierdo, APAGAR_MOTOR);
            break;
    }
}