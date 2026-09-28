#include "mpu6050.h"
#include <Wire.h>
#include <Arduino.h>
#include <math.h>
#include "constantes.h"

// Dirección I2C del MPU6050
#define MPU6050_ADDR   0x68
// Registro de control de energía (escribir 0x00 para despertar el chip)
#define REG_PWR_MGMT_1 0x6B
// Registro de inicio de datos de acelerómetro
#define REG_ACCEL_XOUT 0x3B

// Variables estáticas: normalizadas en unidades g
float ControladorAcelerometro::x = 0.0f;
float ControladorAcelerometro::y = 0.0f;
float ControladorAcelerometro::z = 0.0f;

bool ControladorAcelerometro::inicializar() {
    // Despertar el MPU6050 escribiendo 0x00 en PWR_MGMT_1
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(REG_PWR_MGMT_1);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) {
        Serial.println(F("[MPU6050] ERROR: No responde en 0x68"));
        return false;
    }
    Serial.println(F("[MPU6050] Inicializado correctamente en 0x68"));
    return true;
}

bool ControladorAcelerometro::leerDatos() {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(REG_ACCEL_XOUT);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    Wire.requestFrom((uint8_t)MPU6050_ADDR, (uint8_t)6);
    if (Wire.available() < 6) {
        return false;
    }

    int16_t raw_x = (int16_t)((Wire.read() << 8) | Wire.read());
    int16_t raw_y = (int16_t)((Wire.read() << 8) | Wire.read());
    int16_t raw_z = (int16_t)((Wire.read() << 8) | Wire.read());

    x = (float)raw_x / FACTOR_ESCALA_ACCEL;
    y = (float)raw_y / FACTOR_ESCALA_ACCEL;
    z = (float)raw_z / FACTOR_ESCALA_ACCEL;
    return true;
}

void ControladorAcelerometro::obtenerPitchRoll(float &pitch, float &roll) {
    // Pitch: inclinación adelante/atrás
    pitch = atan2f(-x, sqrtf((y * y) + (z * z))) * RAD_A_GRADOS;
    // Roll: inclinación lateral
    roll  = atan2f(y, z) * RAD_A_GRADOS;
}