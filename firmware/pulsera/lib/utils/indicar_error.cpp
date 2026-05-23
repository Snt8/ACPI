//Incluimos el archivo de cabecera
#include "indicar_error.h"
#include "../motores/motor.h"
//Incluimos constantes y parametros de configuracion
#include "../include/constantes.h"
#include "../include/config.h"
//Incluimos la cabecera de Arduino
#include <Arduino.h>

//Desarrollamos la logica de la funcion
void indicarError() {
    //Repetiriamos el pulso 3 veces para indicar el error
    for (int i = 0; i < 3; i++) {
        //Ejecutamos los 3 pulsos en intervalos
        ControladorMotores::vibrarMotor(ControladorMotores::AMBOS, INTENSIDAD_MOTOR);
        delay(INTERVALOS_ERROR);
        ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
        delay(INTERVALOS_ERROR);
    }

    //Bloqueamos la ejecucion
    while (true) {
        //Detendriamos la ejecucion si hay un error de inicializacion
    }
}