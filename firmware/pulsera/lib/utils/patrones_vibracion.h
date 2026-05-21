#ifndef PULSERA_PATRONES_VIBRACION_H
#define PULSERA_PATRONES_VIBRACION_H
#include "../motores/motor.h"

//Definimos la funcion que controlara los patrones de vibracion de la pulsera
void controlarVibracion(unsigned long milisegundos, unsigned long &ultimo_tiempo_vibracion, bool &motor_encendido, ControladorMotores::Motor motor, uint16_t intensidad);
#endif