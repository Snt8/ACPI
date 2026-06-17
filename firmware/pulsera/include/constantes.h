#ifndef CONSTANTES_H
#define CONSTANTES_H

#include <math.h>

// ── Matemáticas circulares ────────────────────────────────────────────────────
#define GRADOS_CIRCUNFERENCIA   360.0f
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define RAD_A_GRADOS            (180.0f / (float)M_PI)
#define GRADOS_A_RAD            ((float)M_PI / 180.0f)

// ── Motores ───────────────────────────────────────────────────────────────────
#define APAGAR_MOTOR            0
// Intensidad reducida para proteger soldaduras (70 % de duty cycle máximo)
#define INTENSIDAD_MOTOR        180
// Motor izquierdo (GPIO 2) requiere +15 de duty para igualar la respuesta táctil
#define INTENSIDAD_MOTOR_IZQUIERDO 195

// ── Acelerómetro MPU6050 (rango ±2g por defecto) ────────────────────────────
// Datasheet: 16384 LSB/g en rango ±2g.
// NOTA: El requisito original indicaba 16600.0, pero el valor correcto del
// datasheet del MPU6050 es 16384.0 LSB/g. Se usa el valor del datasheet.
#define FACTOR_ESCALA_ACCEL     16384.0f

// ── Calibración Hard-Iron (Leaky Integrator) ─────────────────────────────────
// Offset_nuevo = (Offset_anterior * LEAKY_ALPHA) + (Lectura_actual * LEAKY_BETA)
// Con LEAKY_ALPHA=0.99 y LEAKY_BETA=0.01 la calibración converge en ~100 ciclos
#define LEAKY_ALPHA             0.99f
#define LEAKY_BETA              0.01f

// ── Safety check de heading (pulsera) ────────────────────────────────────────
// Si el heading cambia más de este valor en menos de UMBRAL_TIEMPO_SEGURIDAD_MS,
// la lectura se invalida y no se emite señal de cruce
#define UMBRAL_DELTA_HEADING    40.0f
#define UMBRAL_TIEMPO_SAFETY_MS 500UL

// ── Frecuencia de muestreo (2 Hz) ────────────────────────────────────────────
#define INTERVALO_MUESTREO_MS   500UL

// ── Patrones de vibración ─────────────────────────────────────────────────────
// ROJO (cruce autorizado): 3 pulsos largos y espaciados → silencio → repite.
// Sensación: lenta y deliberada ("cruza ahora, con calma").
// Ciclo total ≈ 3 800 ms.
#define DURACION_PULSO_ROJO_MS      300UL   // pulso largo
#define PAUSA_ROJO_MS               700UL   // espacio entre pulsos
#define SILENCIO_ROJO_MS           1000UL   // pausa antes de repetir

// VERDE (esperar): 3 pulsos rápidos → 3 s de silencio → repite.
// Sensación: ritmo rápido ("todavía no, todavía no").
// Ciclo total ≈ 4 100 ms.
#define DURACION_PULSO_VERDE_MS     100UL   // pulso corto
#define PAUSA_VERDE_MS              500UL   // espacio entre pulsos
#define SILENCIO_VERDE_MS          3000UL   // silencio largo entre grupos

// Direccional (usuario mal orientado): un pulso corto cada ~900 ms.
#define DURACION_PULSO_DIR_MS       200UL
#define PAUSA_DIR_MS                700UL

#endif // CONSTANTES_H
