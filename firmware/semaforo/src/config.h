 #ifndef CONFIG_SEMAFORO_H
#define CONFIG_SEMAFORO_H

// ── Serial ──────────────────────────────────────────────────────────────────
#define BAUD_RATE 115200

// ── I2C — QMC6308 ───────────────────────────────────────────────────────────
// El ESP32 DevKit V1 usa los pines I2C por defecto del framework Arduino:
// SDA = GPIO 21, SCL = GPIO 22
// Dirección I2C del QMC6308: 0x2C
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

// ── LEDs de simulación del semáforo ─────────────────────────────────────────
// LED_ROJO encendido  → semáforo en rojo para autos  → peatón puede cruzar
// LED_VERDE encendido → semáforo en verde para autos → peatón debe esperar
#define PIN_LED_ROJO  25
#define PIN_LED_VERDE 26

// ── Tiempos del ciclo del semáforo (milisegundos) ───────────────────────────
// 25 s por luz: cada fase dura menos que los 30 s sin paquetes que mandan al
// brazalete a standby (TIEMPO_SIN_PAQUETES_STANDBY_MS en su constantes.h).
#define DURACION_VERDE_MS 25000UL   // 25 s verde para autos (peatón espera)
#define DURACION_ROJO_MS  25000UL   // 25 s rojo para autos  (peatón puede cruzar)

// ── Comunicación ESP-NOW ─────────────────────────────────────────────────────
// Intervalo de envío del paquete de estado al brazalete.
// Sincronizado con el ciclo de muestreo del brazalete (2Hz = 500ms).
#define INTERVALO_ENVIO_MS 500UL    // Cada 500 ms (2 Hz)

// ── Calibración de la brújula (ver brujula/calibracion_magnetica.h) ─────────
// El QMC6308 en su rango por defecto (±30 G) da ~1000 LSB/G: el campo
// horizontal de Colombia (~0.30 G) hace recorrer a X e Y ~600 LSB en una vuelta.
#define CALIBRACION_SPAN_MINIMO_LSB  400.0f
#define CALIBRACION_ESTABLE_MS       5000UL
#define CALIBRACION_INTERVALO_MS     50UL
// Al arrancar sin calibración guardada se dan estos ms para calibrar; si no se
// completa, el semáforo funciona igual con el rumbo sin calibrar
#define CALIBRACION_AL_ARRANCAR_MS   30000UL
// Con 'c' por el monitor serie se borra y se calibra con este tiempo máximo
#define CALIBRACION_MANUAL_MS        90000UL

// ── Brújula ──────────────────────────────────────────────────────────────────
// Intervalo de lectura del magnetómetro
#define INTERVALO_LECTURA_BRUJULA_MS 500UL
// El rumbo enviado es el promedio circular de estas lecturas (10 × 500 ms = 5 s)
#define LECTURAS_PROMEDIO 10

#endif // CONFIG_SEMAFORO_H
