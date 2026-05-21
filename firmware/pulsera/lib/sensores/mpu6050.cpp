//Incluimos el archivo de cabecera
#include "mpu6050.h"
//Importamos la bibliotecas para controlar el I2C
#include <Wire.h>
#include <Arduino.h>
//Importamos otras bibliotecas necesarias
#include <math.h>
//Definimos constantes globales
#define FACTOR_ESCALA 16384.0
#define RAD_A_GRADOS (180.0 / M_PI)

float ControladorAcelerometro::x = 0.0;
float ControladorAcelerometro::y = 0.0;
float ControladorAcelerometro::z = 0.0;

bool ControladorAcelerometro::inicializar() {
    //Iniamos el bus I2C
    Wire.begin();
    //Establecemos la direccion para la comunicacion
    Wire.beginTransmission(0x68);
    //Definimos que registros vamos a escribir y que valor escribimos
    Wire.write(0x6B);
    Wire.write(0x00);
    //Comprobamos si la informacion fue enviada exitosamente
    if (Wire.endTransmission() != 0) {
        return false;
    }
    return true;
}

void ControladorAcelerometro::leerDatos() {
    //Establecemos la direccion para leer el registro de los ejes
    Wire.beginTransmission(0x68);
    //Definimos el registro
    Wire.write(0x3B);
    //Comprobamos si la informacion fue obtenida exitosamente
    if (Wire.endTransmission() != 0) {
        return;
    }
    //Pedimos los bytes
    Wire.requestFrom(0x68, 6);
    //Leemos los datos que obtenemos
    int16_t lecturas[3];
    for (int indice = 0; indice < 3; indice++) {
        lecturas[indice] = (Wire.read() << 8) | Wire.read();
    }
    //Asignamos a los atributos el valor de la lectura
    x = lecturas[0] / FACTOR_ESCALA;
    y = lecturas[1] / FACTOR_ESCALA;
    z = lecturas[2] / FACTOR_ESCALA;
}

void ControladorAcelerometro::obtenerPitchRoll(float &pitch, float &roll) {
    //Calculamso la inclinacion hacia adelante y atras
    pitch = atan2(-x, sqrt((y*y) + (z*z))) * RAD_A_GRADOS;
    //Calculamos la inclinacion lateral
    roll = atan2(y, z) * RAD_A_GRADOS;
}