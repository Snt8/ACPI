#ifndef CONFIG_H
#define CONFIG_H

// ── Serial ────────────────────────────────────────────────────────────────────
#define BAUD_RATE 115200

// ── Pines (PCB v3.5, ESP32-C3 Super Mini) ────────────────────────────────────
#define PIN_I2C_SDA             8
#define PIN_I2C_SCL             9
// Lados vistos con el brazalete puesto: el motor de GPIO3 (Q2, conector CN3)
// queda a la derecha y el de GPIO10 (Q1, CN2) a la izquierda. Comprobado en la
// placa con la prueba 03 el 28 oct 2026; antes estaban al revés.
#define PIN_MOTOR_DERECHO       3
#define PIN_MOTOR_IZQUIERDO     10

// ── Direcciones I2C ───────────────────────────────────────────────────────────
// Confirmarlas con los sketches de pruebas/ (01_escaneo_i2c, 02_identificar_sensores)
// y actualizar aquí si el escaneo encuentra otra cosa.
#define DIRECCION_ACELEROMETRO  0x68   // MPU6500 (WHO_AM_I 0x70) con AD0 a GND; confirmado en la placa
#define DIRECCION_MAGNETOMETRO  0x0D   // QMC5883L (GY-271); confirmado en la placa

// ── Lógica de comparación diferencial ────────────────────────────────────────
// Condición de cruce seguro: |Δθ - 180°| < UMBRAL_CRUCE_GRADOS
// donde Δθ = min(|Hp - Hs|, 360 - |Hp - Hs|)
// Los nodos se ENFRENTAN cuando el usuario está correctamente orientado para cruzar.
// 5° desde el 28 oct 2026 (antes 30°): el aviso de la luz sólo llega con la
// persona alineada con el cruce. Exige la brújula calibrada (ver brujula.h).
#define UMBRAL_CRUCE_GRADOS     5.0f

// ── Montaje de los sensores ──────────────────────────────────────────────────
// Giro del acelerómetro respecto al magnetómetro alrededor del eje Z (0, 90,
// 180 o 270). En la PCB v3.5 el módulo MPU está rotado 270° y el GY-271 90°,
// así que sus ejes pueden no coincidir. La calibración lo DETECTA SOLA y lo
// guarda (ver brujula/ejes_acelerometro.h, mensaje "[CALIBRACION]
// Acelerometro girado ..."): este valor sólo se usa mientras no haya una
// calibración hecha con esa detección.
#define ROTACION_ACELEROMETRO_GRADOS 0

// Grados que se suman al rumbo medido para que 0 sea "la persona mira al
// norte" con el brazalete puesto. Corrige que el eje X del magnetómetro no
// apunte exactamente hacia adelante en la muñeca. Cómo medirlo: de pie,
// mirando en la misma dirección que la brújula del celular, comparar con
// [HDG] Brazalete=; CORRECCION = celular − brazalete.
#define CORRECCION_MONTAJE_GRADOS 0.0f

// ── Modo standby ─────────────────────────────────────────────────────────────
// 1: entre escuchas la CPU duerme en light sleep (menor consumo). Sólo se usa
// sin cable USB conectado, porque el light sleep corta el monitor serie.
// 0: espera con delay() aunque no haya USB (útil para depurar con UART).
#define STANDBY_LIGHT_SLEEP     1

#endif // CONFIG_H