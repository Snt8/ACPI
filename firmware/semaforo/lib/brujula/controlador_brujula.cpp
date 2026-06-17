#include "controlador_brujula.h"
#include <Wire.h>
#include <Arduino.h>
#include <math.h>
#include "const.h"
#include "config.h"   // LEAKY_ALPHA, LEAKY_BETA

// ── Mapa de registros QMC6308 ─────────────────────────────────────────────────
#define QMC6308_ADDR       0x2C   // Dirección I2C por defecto
#define QMC6308_REG_DATA   0x01   // Primer registro de datos (6 bytes: Xl,Xh,Yl,Yh,Zl,Zh)
#define QMC6308_REG_CTRL2  0x0A   // Registro de control 2 (modo de operación)
#define QMC6308_REG_CTRL3  0x0D   // Registro de control 3 (soft reset)

#define QMC6308_MODE_CONT  0x03   // Modo continuo, 200Hz
#define QMC6308_SOFT_RESET 0x01   // Soft reset

// ── Variables estáticas ───────────────────────────────────────────────────────
float ControladorBrujulaSemaforo::x          = 0.0f;
float ControladorBrujulaSemaforo::y          = 0.0f;
float ControladorBrujulaSemaforo::z          = 0.0f;
float ControladorBrujulaSemaforo::offset_x   = 0.0f;
float ControladorBrujulaSemaforo::offset_y   = 0.0f;
float ControladorBrujulaSemaforo::offset_z   = 0.0f;
float ControladorBrujulaSemaforo::headingActual = 0.0f;

// ── Escáner I2C de diagnóstico ────────────────────────────────────────────────
// Se ejecuta UNA vez al boot para reportar la dirección real del sensor.
static void escanearI2C_semaforo() {
    Serial.println(F("[SEMAFORO I2C SCAN] Buscando dispositivos..."));
    uint8_t encontrados = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[SEMAFORO I2C SCAN] Dispositivo en 0x%02X\n", addr);
            encontrados++;
        }
    }
    if (encontrados == 0) {
        Serial.println(F("[SEMAFORO I2C SCAN] No se encontraron dispositivos."));
    }
}

// ── Inicialización ────────────────────────────────────────────────────────────
bool ControladorBrujulaSemaforo::inicializar() {
    Wire.begin();

    // Paso 0: escanear bus I2C para confirmar dirección física real del QMC6308
    escanearI2C_semaforo();

    // Paso 1: Soft reset (escribe 0x01 en reg 0x0D)
    Wire.beginTransmission(QMC6308_ADDR);
    Wire.write(QMC6308_REG_CTRL3);
    Wire.write(QMC6308_SOFT_RESET);
    if (Wire.endTransmission() != 0) {
        Serial.println(F("[SEMAFORO QMC6308] ERROR: No responde en 0x2C (soft reset)"));
        return false;
    }
    delay(10);  // Esperar que el reset se complete

    // Paso 2: Configurar modo continuo (escribe 0x03 en reg 0x0A)
    Wire.beginTransmission(QMC6308_ADDR);
    Wire.write(QMC6308_REG_CTRL2);
    Wire.write(QMC6308_MODE_CONT);
    if (Wire.endTransmission() != 0) {
        Serial.println(F("[SEMAFORO QMC6308] ERROR: Fallo al configurar modo continuo"));
        return false;
    }

    Serial.println(F("[SEMAFORO QMC6308] Inicializado correctamente en 0x2C"));
    return true;
}

// ── Lectura con calibración hard-iron y cálculo de heading ───────────────────
bool ControladorBrujulaSemaforo::leerHeading() {
    // Apuntar al registro de datos
    Wire.beginTransmission(QMC6308_ADDR);
    Wire.write(QMC6308_REG_DATA);
    if (Wire.endTransmission(false) != 0) {  // repeated start
        return false;
    }

    // Solicitar 6 bytes
    Wire.requestFrom((uint8_t)QMC6308_ADDR, (uint8_t)6);
    if (Wire.available() < 6) {
        return false;
    }

    // QMC6308 entrega los datos LSB primero (Little-Endian por eje)
    // Orden: Xl, Xh, Yl, Yh, Zl, Zh
    int16_t raw_x = (int16_t)((Wire.read()) | (Wire.read() << 8));
    int16_t raw_y = (int16_t)((Wire.read()) | (Wire.read() << 8));
    int16_t raw_z = (int16_t)((Wire.read()) | (Wire.read() << 8));

    float mx = (float)raw_x;
    float my = (float)raw_y;
    float mz = (float)raw_z;

    // ── Calibración hard-iron: Leaky Integrator ───────────────────────────────
    // Los offsets convergen gradualmente hacia el bias magnético ambiental.
    // LEAKY_ALPHA y LEAKY_BETA definidos en config.h del semáforo.
    offset_x = (offset_x * LEAKY_ALPHA) + (mx * LEAKY_BETA);
    offset_y = (offset_y * LEAKY_ALPHA) + (my * LEAKY_BETA);
    offset_z = (offset_z * LEAKY_ALPHA) + (mz * LEAKY_BETA);

    // Aplicar corrección hard-iron
    x = mx - offset_x;
    y = my - offset_y;
    z = mz - offset_z;

    // ── Heading directo (sin tilt compensation — semáforo fijo y horizontal) ──
    // El factor de escala se cancela en atan2(y, x), no es necesario dividir.
    float angulo = atan2f(y, x) * RAD_A_GRADOS;

    // Normalizar al rango [0, 360)
    if (angulo < 0.0f) {
        angulo += GRADOS_CIRCUNFERENCIA;
    }

    headingActual = angulo;
    return true;
}
