#include "qmc5883l.h"
#include <Wire.h>
#include <Arduino.h>
#include <math.h>
#include "constantes.h"

// ── Mapa de registros QMC6308 ─────────────────────────────────────────────────
#define QMC6308_ADDR        0x2C  // Dirección I2C por defecto
#define QMC6308_REG_DATA    0x01  // Primer registro de datos (6 bytes: Xl,Xh,Yl,Yh,Zl,Zh)
#define QMC6308_REG_CTRL2   0x0A  // Registro de control 2 (modo de operación)
#define QMC6308_REG_CTRL3   0x0D  // Registro de control 3 (soft reset)

// Modo continuo: bit[0:1] = 01 (continuous) | bit[2:3] = 00 (200Hz) → 0x03
#define QMC6308_MODE_CONT   0x03
// Soft reset: bit[0] = 1
#define QMC6308_SOFT_RESET  0x01

// ── Variables estáticas ───────────────────────────────────────────────────────
float ControladorMagnetometro::x        = 0.0f;
float ControladorMagnetometro::y        = 0.0f;
float ControladorMagnetometro::z        = 0.0f;
float ControladorMagnetometro::offset_x = 0.0f;
float ControladorMagnetometro::offset_y = 0.0f;
float ControladorMagnetometro::offset_z = 0.0f;

// ── Escáner I2C de diagnóstico ────────────────────────────────────────────────
// Se ejecuta UNA vez al boot para reportar la dirección real del sensor.
static void escanearI2C() {
    Serial.println(F("[I2C SCAN] Buscando dispositivos..."));
    uint8_t encontrados = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C SCAN] Dispositivo en 0x%02X\n", addr);
            encontrados++;
        }
    }
    if (encontrados == 0) {
        Serial.println(F("[I2C SCAN] No se encontraron dispositivos."));
    }
}

// ── Inicialización ────────────────────────────────────────────────────────────
bool ControladorMagnetometro::inicializar() {
    // Paso 0: escanear bus I2C para confirmar dirección física real
    escanearI2C();

    // Paso 1: Soft reset (escribe 0x01 en reg 0x0D)
    Wire.beginTransmission(QMC6308_ADDR);
    Wire.write(QMC6308_REG_CTRL3);
    Wire.write(QMC6308_SOFT_RESET);
    if (Wire.endTransmission() != 0) {
        Serial.println(F("[QMC6308] ERROR: No responde en 0x2C (soft reset)"));
        return false;
    }
    delay(10);  // Esperar que el reset se complete

    // Paso 2: Configurar modo continuo (escribe 0x03 en reg 0x0A)
    Wire.beginTransmission(QMC6308_ADDR);
    Wire.write(QMC6308_REG_CTRL2);
    Wire.write(QMC6308_MODE_CONT);
    if (Wire.endTransmission() != 0) {
        Serial.println(F("[QMC6308] ERROR: Fallo al configurar modo continuo"));
        return false;
    }

    Serial.println(F("[QMC6308] Inicializado correctamente en 0x2C"));
    return true;
}

// ── Lectura y calibración hard-iron ──────────────────────────────────────────
bool ControladorMagnetometro::leerPosicion() {
    Wire.beginTransmission(QMC6308_ADDR);
    Wire.write(QMC6308_REG_DATA);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    Wire.requestFrom((uint8_t)QMC6308_ADDR, (uint8_t)6);
    if (Wire.available() < 6) {
        return false;
    }

    int16_t raw_x = (int16_t)((Wire.read()) | (Wire.read() << 8));
    int16_t raw_y = (int16_t)((Wire.read()) | (Wire.read() << 8));
    int16_t raw_z = (int16_t)((Wire.read()) | (Wire.read() << 8));

    float mx = (float)raw_x;
    float my = (float)raw_y;
    float mz = (float)raw_z;

    offset_x = (offset_x * LEAKY_ALPHA) + (mx * LEAKY_BETA);
    offset_y = (offset_y * LEAKY_ALPHA) + (my * LEAKY_BETA);
    offset_z = (offset_z * LEAKY_ALPHA) + (mz * LEAKY_BETA);

    x = mx - offset_x;
    y = my - offset_y;
    z = mz - offset_z;
    return true;
}

// ── Cálculo de heading con tilt compensation ──────────────────────────────────
// gx, gy, gz: componentes de gravedad normalizadas en unidades g (de MPU6050).
// Fórmula especificada en el requisito SPIM:
//   x_h = mx*(1 - gx²) + my*(-gx*gy) + mz*(-gx*gz)
//   y_h = mx*(-gy*gx)  + my*(1 - gy²) + mz*(-gy*gz)
//   heading = atan2(y_h, x_h) * (180/PI)  → normalizado a [0, 360)
float ControladorMagnetometro::calcularHeading(float gx, float gy, float gz) {
    // Proyectar campo magnético al plano horizontal compensando la inclinación
    float x_h = (x * (1.0f - gx * gx))
              + (y * (-gx * gy))
              + (z * (-gx * gz));

    float y_h = (x * (-gy * gx))
              + (y * (1.0f - gy * gy))
              + (z * (-gy * gz));

    // Calcular ángulo azimut y convertir a grados
    float heading = atan2f(y_h, x_h) * RAD_A_GRADOS;

    // Normalizar al rango [0, 360)
    if (heading < 0.0f) {
        heading += GRADOS_CIRCUNFERENCIA;
    }

    return heading;
}