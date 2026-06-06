#ifndef CONFIG_SEMAFORO_H
#define CONFIG_SEMAFORO_H

// ── Serial ──────────────────────────────────────────────────────────────────
#define BAUD_RATE 115200

// ── I2C — QMC5883L ──────────────────────────────────────────────────────────
// El ESP32 DevKit V1 usa los pines I2C por defecto del framework Arduino:
// SDA = GPIO 21, SCL = GPIO 22
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
// Intervalo de envío del paquete de estado a la pulsera
#define INTERVALO_ENVIO_MS 200UL    // Cada 200 ms

// ── Brújula ──────────────────────────────────────────────────────────────────
// Intervalo de lectura del magnetómetro
#define INTERVALO_LECTURA_BRUJULA_MS 500UL

#endif // CONFIG_SEMAFORO_H
