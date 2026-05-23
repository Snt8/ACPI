//Incluimos la cabecera
#include "calcular_direccion.h"
//Incluimos bibliotecas necesarias
#include <Arduino.h>
//Incluimos las constantes de configuracion
#include "../include/constantes.h"

float calcularDireccion(float posicionSemaforo, float headingActual) {
    float diferencia = fmod(posicionSemaforo - headingActual + GRADOS_CIRCUNFERENCIA, GRADOS_CIRCUNFERENCIA);
    if (diferencia > GRADOS_CIRCUNFERENCIA / 2) {
        diferencia -= GRADOS_CIRCUNFERENCIA;
    }
    return diferencia;
}