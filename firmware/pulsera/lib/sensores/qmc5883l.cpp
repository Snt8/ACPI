//Incluimos el archivo de cabecera
#include "qmc5883l.h"
//Incluimos bibliotecas necesarias
#include <Wire.h>
#include <Arduino.h>
#include <math.h>
//Incluimos constantes de configuracion
#include "../include/constantes.h"

float ControladorMagnetometro::x = 0.0;
float ControladorMagnetometro::y = 0.0;
float ControladorMagnetometro::z = 0.0;

bool ControladorMagnetometro::inicializar() {
    //Iniciamos la comunicacion con el sensor
    Wire.begin();
    Wire.beginTransmission(0x0D);
    //Configuramos el registro y escribimos el valor
    Wire.write(0x09);
    Wire.write(0x1D);
    //Evaluamos si la informacion se ha enviado correctamente
    if (Wire.endTransmission() != 0) {
        return false;
    }
    return true;
}

void ControladorMagnetometro::leerPosicion() {
    //Iniciamos la comunicacion con el sensor
    Wire.beginTransmission(0x0D);
    //Definimos el registro
    Wire.write(0x00);
    //Comprobamos si los datos fueron obtenidos con exito
    if (Wire.endTransmission() != 0) {
        return;
    }
    //Solicitamos la informacion de los ejes
    Wire.requestFrom(0x0D, 6);
    /* Leemos los datos que obtenemos
     * Iteramos para combinar ambos conjuntos de 2 bytes
     * transformamos a radianes y posteriormente a grados
     */
    int16_t lecturas[3];
    for (int indice = 0; indice < 3; indice++) {
        lecturas[indice] = (Wire.read() << 8) | Wire.read();
    }
    x = lecturas[0] / FACTOR_ESCALA;
    y = lecturas[1] / FACTOR_ESCALA;
    z = lecturas[2] / FACTOR_ESCALA;
}

float ControladorMagnetometro::calcularHeading(const float pitch, const float roll) {
    //Convertimos los datos que obtenemos del acelerometro a radianes
    float pitchRad = pitch * GRADOS_A_RAD;
    float rollRad = roll * GRADOS_A_RAD;
    //Calculamos los ejes corregidos
    float Xh = (x * cos(pitchRad)) + (z * sin(pitchRad));
    float Yh = x * sin(rollRad) * sin(pitchRad) + y * cos(rollRad) - z * sin(rollRad) * cos(pitchRad);
    //Calculamos el angulo de inclinacion y convertimos a grados
    float angulo_corregido = atan2(Yh, Xh) * RAD_A_GRADOS;
    //Evaluamos si el resultado es negativo
    if (angulo_corregido < 0) {
        angulo_corregido += 360;
    }

    return angulo_corregido;
}