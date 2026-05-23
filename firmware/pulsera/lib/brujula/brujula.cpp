//Incluimos la cabecera
#include "brujula.h"
//Incluimos la cabecera de los sensores
#include "../sensores/mpu6050.h"
#include "../sensores/qmc5883l.h"
//Incluimos la cabecera con la funcion para determinar la orientacion del semaforo
#include "../utils/calcular_direccion.h"
//Incluimos otras bibliotecas necesarias
#include <Arduino.h>
//Incluimos parametros de configuracion
#include "config.h"

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
    float diferencia = calcularDireccion(posicionSemaforo, headingActual);
    //Evaluamos si la diferencia es mayor al umbral del semaforo
    if (fabs(diferencia) > UMBRAL_SEMAFORO) {
        return false;
    }
    return true;
}
