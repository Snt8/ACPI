#include "qmc5883l.h"
#include <Wire.h>
#include <Arduino.h>
#include <math.h>
#include "constantes.h"

// ── Mapa de registros QMC5883L ────────────────────────────────────────────────
#define QMC5883L_ADDR        0x0D  // Dirección I2C fija
#define QMC5883L_REG_DATA    0x00  // Primer registro de datos (6 bytes: Xl,Xh,Yl,Yh,Zl,Zh)
#define QMC5883L_REG_CTRL1   0x09  // Control 1: OSR | RNG | ODR | MODE
#define QMC5883L_REG_CTRL2   0x0A  // Control 2: soft reset, roll-over, interrupción
#define QMC5883L_REG_PERIODO 0x0B  // Periodo SET/RESET
#define QMC5883L_REG_CHIP_ID 0x0D  // Identificación del chip (debe leer 0xFF)

// Control 1: OSR[7:6] = 00 (512) | RNG[5:4] = 01 (±8 G) | ODR[3:2] = 11 (200 Hz) | MODE[1:0] = 01 (continuo) → 0x1D
// Se usa ±8 G en vez de ±2 G para no saturar con el imán de los motores de vibración cercanos.
#define QMC5883L_MODO_CONT   0x1D
// Control 2: bit[7] = 1 (soft reset)
#define QMC5883L_SOFT_RESET  0x80
// Valor recomendado por el datasheet para el periodo SET/RESET
#define QMC5883L_PERIODO_REC 0x01
#define QMC5883L_ID_ESPERADO 0xFF

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

// ── Escritura de un registro ──────────────────────────────────────────────────
static bool escribirRegistro(uint8_t reg, uint8_t valor) {
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(reg);
    Wire.write(valor);
    return Wire.endTransmission() == 0;
}

// ── Inicialización ────────────────────────────────────────────────────────────
bool ControladorMagnetometro::inicializar() {
    // Paso 0: escanear bus I2C para confirmar dirección física real
    escanearI2C();

    // Paso 1: Soft reset (escribe 0x80 en reg 0x0A)
    if (!escribirRegistro(QMC5883L_REG_CTRL2, QMC5883L_SOFT_RESET)) {
        Serial.println(F("[QMC5883L] ERROR: No responde en 0x0D (soft reset)"));
        return false;
    }
    delay(10);  // Esperar que el reset se complete

    // Paso 2: Verificar chip ID (solo aviso: algunos clones no lo implementan)
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(QMC5883L_REG_CHIP_ID);
    if (Wire.endTransmission(false) == 0 &&
        Wire.requestFrom((uint8_t)QMC5883L_ADDR, (uint8_t)1) == 1) {
        uint8_t id = Wire.read();
        if (id != QMC5883L_ID_ESPERADO) {
            Serial.printf("[QMC5883L] AVISO: chip ID 0x%02X (se esperaba 0xFF)\n", id);
        }
    }

    // Paso 3: Periodo SET/RESET (escribe 0x01 en reg 0x0B)
    if (!escribirRegistro(QMC5883L_REG_PERIODO, QMC5883L_PERIODO_REC)) {
        Serial.println(F("[QMC5883L] ERROR: Fallo al configurar periodo SET/RESET"));
        return false;
    }

    // Paso 4: Configurar modo continuo (escribe 0x1D en reg 0x09)
    if (!escribirRegistro(QMC5883L_REG_CTRL1, QMC5883L_MODO_CONT)) {
        Serial.println(F("[QMC5883L] ERROR: Fallo al configurar modo continuo"));
        return false;
    }

    Serial.println(F("[QMC5883L] Inicializado correctamente en 0x0D"));
    return true;
}

// ── Lectura y calibración hard-iron ──────────────────────────────────────────
bool ControladorMagnetometro::leerPosicion() {
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(QMC5883L_REG_DATA);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    Wire.requestFrom((uint8_t)QMC5883L_ADDR, (uint8_t)6);
    if (Wire.available() < 6) {
        return false;
    }

    // Lecturas en sentencias separadas: el orden de evaluación de los operandos
    // de "|" no está definido en C++ y podría invertir LSB y MSB.
    uint8_t datos[6];
    for (uint8_t i = 0; i < 6; i++) {
        datos[i] = Wire.read();
    }
    int16_t raw_x = (int16_t)(datos[0] | (datos[1] << 8));
    int16_t raw_y = (int16_t)(datos[2] | (datos[3] << 8));
    int16_t raw_z = (int16_t)(datos[4] | (datos[5] << 8));

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