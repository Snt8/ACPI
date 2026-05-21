//Incluimos la libreria de arduino
#include <Arduino.h>
//Inclumos el archivo de cabecera
#include "patrones_vibracion.h"
//Incluimos motores/motor.h para utilizar la coleccion Motor
#include "../motores/motor.h"

void controlarVibracion(unsigned long milisegundos, unsigned long &ultimo_tiempo_vibracion, bool &motor_encendido, ControladorMotores::Motor motor, uint16_t intensidad) {
    unsigned long tiempo_actual = millis();
    //Evaluamos si el tiempo transcurrido desde la ultima vez es el estipulado
    if (((tiempo_actual - ultimo_tiempo_vibracion) >= milisegundos) && !motor_encendido) {
        ControladorMotores::vibrarMotor(motor, intensidad);
        motor_encendido = true;
        ultimo_tiempo_vibracion = tiempo_actual;
    }

    if (((tiempo_actual - ultimo_tiempo_vibracion >= milisegundos)) && motor_encendido) {
        ControladorMotores::detenerMotor(motor);
        motor_encendido = false;
        ultimo_tiempo_vibracion = tiempo_actual;
    }
}