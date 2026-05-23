//Incluimos la cabecera
#include "motor.h"
//Incluimos otras bibliotecas necesarias
#include <stdint.h>
#include <Arduino.h>
//Incluimos constantes de configuracion
#include "../include/constantes.h"

bool ControladorMotores::inicializar() {
    //Ejecutamos ledcSetup para inicializar el motor derecho, si falla; retornamos
    if (ledcSetup(canal_motor_derecho, frecuencia, resolucion) == 0) {
        return false;
    }
    //Conectamos el pin del motor derecho
    ledcAttachPin(pin_motor_derecho, canal_motor_derecho);
    //Ejecutamos ledcSetup par inicializar ahora el motor izquierdo
    if (ledcSetup(canal_motor_izquierdo, frecuencia, resolucion) == 0) {
        return false;
    }
    //Conectamos el pin del motor izquierdo
    ledcAttachPin(pin_motor_izquierdo, canal_motor_izquierdo);
    //Ejecutamos ledcSetup para inicializar el motor izquierdo, si falla; retornamos
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

