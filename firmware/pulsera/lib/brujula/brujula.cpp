//Incluimos la cabecera
#include "brujula.h"
//Incluimos la cabecera de los sensores
#include "../sensores/mpu6050.h"
#include "../sensores/qmc5883l.h"
//Incluimos otras bibliotecas necesarias
#include <Arduino.h>
//Definimos constantes globales
#define GRADOS_CIRCUNFERENCIA 360.0
#define UMBRAL_SEMAFORO 45.0

//Inicializamos el headingActual
float ControladorBrujula::headingActual = 0.0;

bool ControladorBrujula::inicializar() {
    //Inicializamos los sensores
    bool acelerometro_inicializado = ControladorAcelerometro::inicializar();
    bool magnetometro_inicializado = ControladorMagnetometro::inicializar();
    //Verificamos que ambos se hayan inicializado correctamente
    if (acelerometro_inicializado && magnetometro_inicializado) {
        return true;
    }
    //Retornamos si resulto mal
    return false;
}

void ControladorBrujula::obtenerHeading() {
    //Definimos las variables para guardar el pitch y roll
    float pitch = 0.0;
    float roll = 0.0;
    //Leemos los datos del acelerometro
    ControladorAcelerometro::leerDatos();
    //Obtenemos el pitch y roll
    ControladorAcelerometro::obtenerPitchRoll(pitch, roll);
    //Leemos los datos del magnetometro
    ControladorMagnetometro::leerPosicion();
    //Calculamos el heading y lo asignamos
    headingActual = ControladorMagnetometro::calcularHeading(pitch, roll);
}

bool ControladorBrujula::revisarSemaforo(uint16_t posicionSemaforo) {
    //Calculamos la diferencia entre la posicion de la pulsera y el semaforo
    float diferencia = fmod(posicionSemaforo - headingActual + GRADOS_CIRCUNFERENCIA, GRADOS_CIRCUNFERENCIA);
    //Si la diferencia es muy amplia, determinamos la menor
    if (diferencia > GRADOS_CIRCUNFERENCIA / 2) {
        diferencia -= GRADOS_CIRCUNFERENCIA;
    }
    //Comparamos la diferencia con el umbral para determinar si ese semaforo es correcto
    if (fabs(diferencia) > UMBRAL_SEMAFORO) {
        return false;
    }
    //Retornamos si determinamos que el semaforo es correcto
    return true;
}
