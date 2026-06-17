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
#define DURACION_VERDE_MS 30000UL   // 30 s verde para autos (peatón espera)
#define DURACION_ROJO_MS  30000UL   // 30 s rojo para autos  (peatón puede cruzar)

// ── Comunicación ESP-NOW ─────────────────────────────────────────────────────
// Intervalo de envío del paquete de estado a la pulsera.
// Sincronizado con el ciclo de muestreo de la pulsera (2Hz = 500ms).
#define INTERVALO_ENVIO_MS 500UL    // Cada 500 ms (2 Hz)

// ── Calibración Hard-Iron (Leaky Integrator) ─────────────────────────────────
// Offset_nuevo = (Offset_anterior * LEAKY_ALPHA) + (Lectura_actual * LEAKY_BETA)
#define LEAKY_ALPHA  0.99f
#define LEAKY_BETA   0.01f

// ── Brújula ──────────────────────────────────────────────────────────────────
// Intervalo de lectura del magnetómetro
#define INTERVALO_LECTURA_BRUJULA_MS 500UL

#endif // CONFIG_SEMAFORO_H
