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
// En la PCB v3.5 el motor ve VBAT (3.7–4.2 V): a 180/255 vibraba tan fuerte que
// amenazaba las soldaduras, y 120 seguía siendo mucho. 100/255 ≈ 39 % de duty.
// Si a este valor un motor no llega a arrancar, verlo con el barrido 'b' de la
// prueba 03, que muestra desde qué duty arranca cada uno.
#define INTENSIDAD_MOTOR        100
// Motor izquierdo (GPIO 3). Antes llevaba +15 para igualar la sensación, un
// valor heredado que nunca se confirmó en esta placa: queda igual al derecho
// hasta medirlo con los dos motores soldados.
#define INTENSIDAD_MOTOR_IZQUIERDO 100

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

// ── Frecuencia de muestreo (2 Hz) ────────────────────────────────────────────
#define INTERVALO_MUESTREO_MS   500UL

// ── Telemetría BLE al móvil ──────────────────────────────────────────────────
#define INTERVALO_TELEMETRIA_BLE_MS 500UL

// ── Safety check de heading (brazalete) ────────────────────────────────────────
// Si el heading cambia más de UMBRAL_DELTA_HEADING entre dos lecturas separadas
// por menos de UMBRAL_TIEMPO_SAFETY_MS, la lectura se invalida (giro brusco de
// la muñeca) y no se indica ningún giro. El umbral de tiempo tiene que ser MAYOR
// que el intervalo de muestreo: con los dos en 500 ms, dos lecturas seguidas
// nunca estaban a menos de 500 ms y el chequeo no se activaba nunca.
// Tras una pausa larga (mientras vibra un motor no se lee la brújula) la primera
// lectura no se compara: la referencia es demasiado vieja.
#define UMBRAL_DELTA_HEADING    40.0f
#define UMBRAL_TIEMPO_SAFETY_MS (2 * INTERVALO_MUESTREO_MS)

static_assert(UMBRAL_TIEMPO_SAFETY_MS > INTERVALO_MUESTREO_MS,
              "El safety check debe abarcar dos muestras consecutivas");

// ── Standby ──────────────────────────────────────────────────────────────────
// Sin paquetes del semáforo durante este tiempo, el brazalete pasa a standby.
// El semáforo envía un paquete cada 500 ms y cambia de luz cada 25 s.
#define TIEMPO_SIN_PAQUETES_STANDBY_MS 30000UL

// En standby la radio sólo escucha una ventana corta cada INTERVALO (inicio a
// inicio); el resto del tiempo el WiFi está apagado. La ventana abarca más de
// dos envíos del semáforo: si el WiFi tarda menos de 200 ms en arrancar,
// escucha al menos dos paquetes.
#define INTERVALO_ESCUCHA_STANDBY_MS   5000UL
#define VENTANA_ESCUCHA_STANDBY_MS     1200UL
#define INTERVALO_ENVIO_SEMAFORO_MS    500UL   // INTERVALO_ENVIO_MS en firmware/semaforo

static_assert(VENTANA_ESCUCHA_STANDBY_MS > 2 * INTERVALO_ENVIO_SEMAFORO_MS,
              "La ventana de escucha debe abarcar al menos dos envios del semaforo");
static_assert(VENTANA_ESCUCHA_STANDBY_MS < INTERVALO_ESCUCHA_STANDBY_MS,
              "La ventana de escucha debe ser mas corta que el intervalo");

// ── Patrones de vibración (ver patrones.md) ──────────────────────────────────
// Cada patrón suena UNA vez por evento y nunca se repite solo. "Pulso" = motor
// encendido a su INTENSIDAD_*; "pausa" = motor apagado entre dos pulsos.

// Regla de protección de las soldaduras: entre el inicio de un patrón y el
// del siguiente pasan al menos estos ms, y dos patrones nunca se solapan.
// La garantiza ControladorPatrones; ningún patrón puede saltársela.
#define SEPARACION_MINIMA_EVENTOS_MS   3000UL

// Además, tras terminar un patrón los motores descansan al menos este tiempo
// antes del siguiente. Importa con los avisos de luz (4.5 s y 2 s): sin esto,
// un cambio de luz durante el verde haría sonar el rojo pegado a él.
#define SILENCIO_TRAS_PATRON_MS        1000UL

// 1. Semáforo detectado: 3 pulsos cortos de ambos motores (1.6 s en total)
#define DETECTADO_PULSOS               3
#define DETECTADO_PULSO_MS             200UL
#define DETECTADO_PAUSA_MS             500UL

// 2. Orientación: 1 pulso del motor del lado hacia el que hay que girar.
//    Se repite cada INTERVALO_GIRO_MS mientras la persona siga mal orientada.
#define GIRO_PULSO_MS                  300UL
#define INTERVALO_GIRO_MS              3000UL

// 3. Cruce permitido (semáforo peatonal en verde): 2 pulsos sostenidos (4.5 s)
#define PERMITIDO_PULSOS               2
#define PERMITIDO_PULSO_MS             2000UL
#define PERMITIDO_PAUSA_MS             500UL

// 4. Cruce prohibido (semáforo peatonal en rojo): 1 pulso sostenido (2 s)
#define PROHIBIDO_PULSOS               1
#define PROHIBIDO_PULSO_MS             2000UL

static_assert(INTERVALO_GIRO_MS >= SEPARACION_MINIMA_EVENTOS_MS,
              "El pulso de orientacion no puede repetirse antes de la separacion minima");

// ── Orientación frente al cruce ──────────────────────────────────────────────
// La persona queda "de frente" cuando |Δθ − 180°| < UMBRAL_CRUCE_GRADOS (config.h)
// y sólo vuelve a "mal orientada" cuando supera UMBRAL + HISTERESIS. Sin esta
// banda, una lectura que oscila alrededor de 30° alternaría los dos estados y
// cada vuelta a "de frente" repetiría el patrón del semáforo sin un evento real.
#define HISTERESIS_ORIENTACION_GRADOS  10.0f

#endif // CONSTANTES_H
