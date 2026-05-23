//Incluimos la cabecera
#include "motor.h"
//Incluimos otras bibliotecas necesarias
#include <stdint.h>
#include <Arduino.h>
//Incluimos constantes de configuracion
#include "constantes.h"

bool ControladorMotores::inicializar() {
    // 1. Configurar los canales (el ESP32-C3 tiene canales del 0 al 5)
    // ledcSetup(canal, frecuencia, resolución)
    if (ledcSetup(canal_motor_derecho, frecuencia, resolucion) == 0) return false;
    if (ledcSetup(canal_motor_izquierdo, frecuencia, resolucion) == 0) return false;

    // 2. Asociar los pines
    ledcAttachPin(pin_motor_derecho, canal_motor_derecho);
    ledcAttachPin(pin_motor_izquierdo, canal_motor_izquierdo);

    // 3. Inicializar en 0
    ledcWrite(canal_motor_derecho, 0);
    ledcWrite(canal_motor_izquierdo, 0);

    return true;
}

void ControladorMotores::vibrarMotor(Motor motor, uint8_t intensidad) {
    //Evaluamos el caso del motor que queremos activar
    switch(motor) {
        case DERECHO:
            //Encendemos el motor derecho
            ledcWrite(canal_motor_derecho, intensidad);
            break;

        case IZQUIERDO:
            //Encendemos el motor izquierdo
            ledcWrite(canal_motor_izquierdo, intensidad);
            break;

        case AMBOS:
            //Encendemos ambos motores
            ledcWrite(canal_motor_derecho, intensidad);
            ledcWrite(canal_motor_izquierdo, intensidad);
            break;
    }
}

void ControladorMotores::detenerMotor(Motor motor) {
    //Evaluamos los casos de los motores para apagarlos
    switch(motor) {
        case DERECHO:
            //Apagamos el motor derecho
            ledcWrite(canal_motor_derecho, APAGAR_MOTOR);
            break;

        case IZQUIERDO:
            //Apagamos el motor izquierdo
            ledcWrite(canal_motor_izquierdo, APAGAR_MOTOR);
            break;

        case AMBOS:
            //Apagamos ambos
            ledcWrite(canal_motor_derecho, APAGAR_MOTOR);
            ledcWrite(canal_motor_izquierdo, APAGAR_MOTOR);
            break;
    }
}

