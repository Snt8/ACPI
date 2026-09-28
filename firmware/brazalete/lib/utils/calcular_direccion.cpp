//Incluimos la cabecera
#include "calcular_direccion.h"
//Incluimos bibliotecas necesarias
#include <Arduino.h>
//Incluimos las constantes de configuracion
#include "constantes.h"

float calcularDireccion(float posicionSemaforo, float headingActual) {
    //Calculamos la diferencia angular positiva usando aritmetica modular
    float diferencia = fmod(posicionSemaforo - headingActual + GRADOS_CIRCUNFERENCIA, GRADOS_CIRCUNFERENCIA);
    //En caso de que el giro sea mayor a 180 grados, resulta mas rapido dar el giro al contrario
    if (diferencia > GRADOS_CIRCUNFERENCIA / 2) {
        diferencia -= GRADOS_CIRCUNFERENCIA; //Convertimos el angulo en su equivalente negativo
    }
    return diferencia;
}