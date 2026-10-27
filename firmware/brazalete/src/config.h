#ifndef CONFIG_H
#define CONFIG_H

// ── Serial ────────────────────────────────────────────────────────────────────
#define BAUD_RATE 115200

// ── Pines (PCB v3.5, ESP32-C3 Super Mini) ────────────────────────────────────
#define PIN_I2C_SDA             8
#define PIN_I2C_SCL             9
#define PIN_MOTOR_DERECHO       10
#define PIN_MOTOR_IZQUIERDO     3

// ── Direcciones I2C ───────────────────────────────────────────────────────────
// Confirmarlas con los sketches de pruebas/ (01_escaneo_i2c, 02_identificar_sensores)
// y actualizar aquí si el escaneo encuentra otra cosa.
#define DIRECCION_ACELEROMETRO  0x68   // MPU6500 (WHO_AM_I 0x70) con AD0 a GND; confirmado en la placa
#define DIRECCION_MAGNETOMETRO  0x0D   // QMC5883L (GY-271); confirmado en la placa

// ── Lógica de comparación diferencial ────────────────────────────────────────
// Condición de cruce seguro: |Δθ - 180°| < UMBRAL_CRUCE_GRADOS
// donde Δθ = min(|Hp - Hs|, 360 - |Hp - Hs|)
// Los nodos se ENFRENTAN cuando el usuario está correctamente orientado para cruzar.
#define UMBRAL_CRUCE_GRADOS     30.0f

// ── Modo standby ─────────────────────────────────────────────────────────────
// 1: entre escuchas la CPU duerme en light sleep (menor consumo). Sólo se usa
// sin cable USB conectado, porque el light sleep corta el monitor serie.
// 0: espera con delay() aunque no haya USB (útil para depurar con UART).
#define STANDBY_LIGHT_SLEEP     1

// ── Timing de vibración de error (indicar_error.cpp) ─────────────────────────
#define INTERVALOS_ERROR        200

#endif // CONFIG_H