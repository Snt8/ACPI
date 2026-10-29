#include "qmc5883l.h"
#include <Wire.h>
#include <Arduino.h>
#include "../config.h"

// ── Mapa de registros QMC5883L ────────────────────────────────────────────────
#define QMC5883L_ADDR        DIRECCION_MAGNETOMETRO  // Ver config.h
#define QMC5883L_REG_DATA    0x00  // Primer registro de datos (6 bytes: Xl,Xh,Yl,Yh,Zl,Zh)
#define QMC5883L_REG_CTRL1   0x09  // Control 1: OSR | RNG | ODR | MODE
#define QMC5883L_REG_CTRL2   0x0A  // Control 2: soft reset, roll-over, interrupción
#define QMC5883L_REG_PERIODO 0x0B  // Periodo SET/RESET
#define QMC5883L_REG_CHIP_ID 0x0D  // Identificación del chip (debe leer 0xFF)

// Control 1: OSR[7:6] = 00 (512) | RNG[5:4] = 01 (±8 G) | ODR[3:2] = 11 (200 Hz) | MODE[1:0] = 01 (continuo) → 0x1D
// Se usa ±8 G en vez de ±2 G para no saturar con el imán de los motores de vibración cercanos.
#define QMC5883L_MODO_CONT   0x1D
// Mismo OSR/RNG/ODR con MODE = 00 (standby): el chip deja de medir
#define QMC5883L_MODO_STANDBY 0x1C
// Control 2: bit[7] = 1 (soft reset)
#define QMC5883L_SOFT_RESET  0x80
// Valor recomendado por el datasheet para el periodo SET/RESET
#define QMC5883L_PERIODO_REC 0x01
#define QMC5883L_ID_ESPERADO 0xFF
// Registro de estado (0x06): bit[1] = OVL, algún eje saturado
#define QMC5883L_ESTADO_OVL  0x02

// ── Escritura de un registro ──────────────────────────────────────────────────
static bool escribirRegistro(uint8_t reg, uint8_t valor) {
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(reg);
    Wire.write(valor);
    return Wire.endTransmission() == 0;
}

// ── Inicialización ────────────────────────────────────────────────────────────
bool ControladorMagnetometro::inicializar() {
    // (El escaneo del bus está en pruebas/01_escaneo_i2c: no se repite en cada
    // arranque ni cada vez que el brazalete sale de standby)

    // Paso 1: Soft reset (escribe 0x80 en reg 0x0A)
    if (!escribirRegistro(QMC5883L_REG_CTRL2, QMC5883L_SOFT_RESET)) {
        Serial.printf("[QMC5883L] ERROR: No responde en 0x%02X (soft reset)\n", QMC5883L_ADDR);
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

    Serial.printf("[QMC5883L] Inicializado correctamente en 0x%02X\n", QMC5883L_ADDR);
    return true;
}

// ── Bajo consumo ─────────────────────────────────────────────────────────────
bool ControladorMagnetometro::dormir() {
    return escribirRegistro(QMC5883L_REG_CTRL1, QMC5883L_MODO_STANDBY);
}

// ── Lectura cruda ────────────────────────────────────────────────────────────
bool ControladorMagnetometro::leerCrudo(float crudo[3]) {
    // Se leen 7 bytes: X, Y, Z y el registro de estado (0x06)
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(QMC5883L_REG_DATA);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    Wire.requestFrom((uint8_t)QMC5883L_ADDR, (uint8_t)7);
    if (Wire.available() < 7) {
        return false;
    }

    // Lecturas en sentencias separadas: el orden de evaluación de los operandos
    // de "|" no está definido en C++ y podría invertir LSB y MSB.
    uint8_t datos[7];
    for (uint8_t i = 0; i < 7; i++) {
        datos[i] = Wire.read();
    }
    // Bit OVL: algún eje se salió del rango (p. ej. un imán pegado al sensor).
    // Esa lectura no sirve ni para el rumbo ni para la calibración.
    if (datos[6] & QMC5883L_ESTADO_OVL) {
        return false;
    }
    crudo[0] = (float)(int16_t)(datos[0] | (datos[1] << 8));
    crudo[1] = (float)(int16_t)(datos[2] | (datos[3] << 8));
    crudo[2] = (float)(int16_t)(datos[4] | (datos[5] << 8));
    return true;
}
