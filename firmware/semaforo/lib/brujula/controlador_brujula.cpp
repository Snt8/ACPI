#include "controlador_brujula.h"
#include <Wire.h>
#include <Arduino.h>
#include <math.h>
#include "const.h"

// Dirección I2C del QMC5883L
#define QMC5883L_ADDR 0x0D

float ControladorBrujulaSemaforo::x = 0.0;
float ControladorBrujulaSemaforo::y = 0.0;
float ControladorBrujulaSemaforo::z = 0.0;
float ControladorBrujulaSemaforo::headingActual = 0.0;

bool ControladorBrujulaSemaforo::inicializar() {
    Wire.begin();
    
    // Configuración del QMC5883L:
    // Registro de Control 1 (0x09): 
    // - Mode: Continuous (0x01)
    // - ODR: 200Hz (0x0C)
    // - RNG: 8G (0x10) // o 2G (0x00)
    // - OSR: 512 (0x00)
    // Combinado: 0x1D (o 0x0D si es 2G). En el código de la pulsera se usa 0x1D.
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(0x09);
    Wire.write(0x1D);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    return true;
}

bool ControladorBrujulaSemaforo::leerHeading() {
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(0x00); // Empezar desde el registro 0x00
    if (Wire.endTransmission() != 0) {
        return false;
    }

    Wire.requestFrom(QMC5883L_ADDR, 6);
    if (Wire.available() < 6) {
        return false;
    }

    // El QMC5883L entrega los datos con LSB primero, pero en la pulsera se leyó como (Wire.read() << 8) | Wire.read().
    // Para consistencia con el firmware de la pulsera, leemos en el mismo formato:
    int16_t raw_x = (Wire.read() << 8) | Wire.read();
    int16_t raw_y = (Wire.read() << 8) | Wire.read();
    int16_t raw_z = (Wire.read() << 8) | Wire.read();

    // Como el semáforo está fijo en un plano horizontal, no requerimos compensación de inclinación (tilt).
    // Calculamos el heading directamente usando atan2.
    // El factor de escala se cancela en atan2(y, x), por lo que no es estrictamente necesario dividir.
    x = (float)raw_x;
    y = (float)raw_y;
    z = (float)raw_z;

    float angulo = atan2(y, x) * RAD_A_GRADOS;
    
    // Ajustar a rango 0-359 grados
    if (angulo < 0) {
        angulo += GRADOS_CIRCUNFERENCIA;
    }
    
    headingActual = angulo;
    return true;
}
