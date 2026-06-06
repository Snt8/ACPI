#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "const.h"
#include "controlador_brujula.h"
#include "comunicador_pulsera.h"

// Estados del semáforo (punto de vista de los autos)
enum class EstadoSemaforoAutos {
    ROJO,  // Autos se detienen -> Peatón puede cruzar (permitidoCruce = 1)
    VERDE  // Autos avanzan    -> Peatón espera (permitidoCruce = 0)
};

// Variables de estado
EstadoSemaforoAutos estadoActualAutos = EstadoSemaforoAutos::VERDE;
unsigned long tiempoInicioEstado = 0;
unsigned long duracionEstadoActual = DURACION_VERDE_MS;

// Timers no bloqueantes
unsigned long ultimoEnvioEspNow = 0;
unsigned long ultimaLecturaBrujula = 0;

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);
    Serial.println("[SEMAFORO] Inicializando...");

    // Inicializar pines de LEDs
    pinMode(PIN_LED_ROJO, OUTPUT);
    pinMode(PIN_LED_VERDE, OUTPUT);
    
    // Estado inicial: Verde para autos (LED verde ON, LED rojo OFF)
    digitalWrite(PIN_LED_ROJO, LOW);
    digitalWrite(PIN_LED_VERDE, HIGH);

    // Inicializar I2C y Brújula
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (!ControladorBrujulaSemaforo::inicializar()) {
        Serial.println("[SEMAFORO] [ERROR] No se pudo inicializar la brújula QMC5883L!");
    } else {
        Serial.println("[SEMAFORO] Brújula inicializada correctamente.");
    }

    // Inicializar ESP-NOW
    if (!ComunicadorPulsera::inicializar()) {
        Serial.println("[SEMAFORO] [ERROR] No se pudo inicializar ESP-NOW!");
    } else {
        Serial.println("[SEMAFORO] ESP-NOW inicializado correctamente.");
    }

    tiempoInicioEstado = millis();
    Serial.println("[SEMAFORO] Inicialización completa. Iniciando ciclo FSM...");
}

void loop() {
    unsigned long ahora = millis();

    // ── 1. Máquina de Estados (FSM) del Semáforo ─────────────────────────────
    if (ahora - tiempoInicioEstado >= duracionEstadoActual) {
        // Cambiar de estado
        if (estadoActualAutos == EstadoSemaforoAutos::VERDE) {
            estadoActualAutos = EstadoSemaforoAutos::ROJO;
            duracionEstadoActual = DURACION_ROJO_MS;
            digitalWrite(PIN_LED_ROJO, HIGH);
            digitalWrite(PIN_LED_VERDE, LOW);
            Serial.println("[SEMAFORO] Estado cambiado a: ROJO (Peatones pueden cruzar)");
        } else {
            estadoActualAutos = EstadoSemaforoAutos::VERDE;
            duracionEstadoActual = DURACION_VERDE_MS;
            digitalWrite(PIN_LED_ROJO, LOW);
            digitalWrite(PIN_LED_VERDE, HIGH);
            Serial.println("[SEMAFORO] Estado cambiado a: VERDE (Autos avanzan, Peatones esperan)");
        }
        tiempoInicioEstado = ahora;
    }

    // ── 2. Lectura de la Brújula (Magnetómetro) ──────────────────────────────
    if (ahora - ultimaLecturaBrujula >= INTERVALO_LECTURA_BRUJULA_MS) {
        ultimaLecturaBrujula = ahora;
        if (ControladorBrujulaSemaforo::leerHeading()) {
            Serial.printf("[SEMAFORO] Heading: %.2f°\n", ControladorBrujulaSemaforo::headingActual);
        } else {
            Serial.println("[SEMAFORO] [ERROR] Falló la lectura de la brújula");
        }
    }

    // ── 3. Envío de Telemetría por ESP-NOW ────────────────────────────────────
    if (ahora - ultimoEnvioEspNow >= INTERVALO_ENVIO_MS) {
        ultimoEnvioEspNow = ahora;

        // Calcular el tiempo restante en segundos
        unsigned long transcurrido = ahora - tiempoInicioEstado;
        uint8_t tiempoRestante = 0;
        if (transcurrido < duracionEstadoActual) {
            tiempoRestante = (uint8_t)((duracionEstadoActual - transcurrido) / 1000UL);
        }

        // Construir el payload de estado
        estado_semaforo payload;
        payload.id = 1; // ID del semáforo en la red local
        payload.grados = (uint16_t)ControladorBrujulaSemaforo::headingActual;
        payload.permitidoCruce = (estadoActualAutos == EstadoSemaforoAutos::ROJO) ? 1 : 0;
        payload.tiempo = tiempoRestante;

        // Enviar vía ESP-NOW (broadcast)
        if (ComunicadorPulsera::enviarEstado(payload)) {
            // Imprimir detalles de debug cada 2 segundos aproximadamente
            static unsigned long ultimaImpresionDebug = 0;
            if (ahora - ultimaImpresionDebug >= 2000UL) {
                ultimaImpresionDebug = ahora;
                Serial.printf("[SEMAFORO] ESP-NOW TX -> Grados: %d, Permitido: %d, Tiempo: %ds\n", 
                              payload.grados, payload.permitidoCruce, payload.tiempo);
            }
        } else {
            Serial.println("[SEMAFORO] [ERROR] Falló el envío ESP-NOW");
        }
    }
}