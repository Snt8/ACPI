// Firmware del semáforo ACPI (ESP32 DevKit V1).
// Arduino IDE: abrir este archivo; el resto del código está en src/ y se compila solo.
// ESP-IDF: main/main.cpp incluye este archivo (ver README.md).
#include <Arduino.h>
#include <Wire.h>
#include "src/config.h"
#include "src/const.h"
#include "src/brujula/controlador_brujula.h"
#include "src/comunicacion/comunicador_brazalete.h"

// Luz del semáforo PEATONAL (la que muestran los LEDs)
enum class LuzPeatonal {
    VERDE,  // El peatón puede cruzar  (LED verde, permitidoCruce = 1)
    ROJO    // El peatón debe esperar  (LED rojo,  permitidoCruce = 0)
};

// Variables de estado: arranca en rojo, lo seguro
LuzPeatonal luzActual = LuzPeatonal::ROJO;
unsigned long tiempoInicioEstado = 0;
unsigned long duracionEstadoActual = DURACION_ROJO_MS;

// Enciende sólo el LED de la luz indicada
void mostrarLuz(LuzPeatonal luz) {
    digitalWrite(PIN_LED_VERDE, luz == LuzPeatonal::VERDE ? HIGH : LOW);
    digitalWrite(PIN_LED_ROJO,  luz == LuzPeatonal::ROJO  ? HIGH : LOW);
}

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
    
    // Estado inicial: rojo peatonal (esperar)
    mostrarLuz(luzActual);

    // Inicializar I2C y Brújula
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (!ControladorBrujulaSemaforo::inicializar()) {
        Serial.println("[SEMAFORO] [ERROR] No se pudo inicializar la brújula QMC6308!");
    } else {
        Serial.println("[SEMAFORO] Brújula inicializada correctamente.");
    }
    // Sin calibración guardada se da un tiempo para calibrar antes de empezar
    if (!ControladorBrujulaSemaforo::calibrada()) {
        ControladorBrujulaSemaforo::calibrar(CALIBRACION_AL_ARRANCAR_MS);
    }

    // Inicializar ESP-NOW
    if (!ComunicadorBrazalete::inicializar()) {
        Serial.println("[SEMAFORO] [ERROR] No se pudo inicializar ESP-NOW!");
    } else {
        Serial.println("[SEMAFORO] ESP-NOW inicializado correctamente.");
    }

    // Watchdog del loop: si loop() deja de volver por más de 5 s (algo se quedó
    // pegado), el ESP32 se reinicia solo. El core lo alimenta en cada vuelta.
    enableLoopWDT();

    tiempoInicioEstado = millis();
    Serial.println("[SEMAFORO] Inicialización completa. Iniciando ciclo FSM...");
}

// 'c' por el monitor serie: borra la calibración de la brújula y calibra de
// nuevo (el watchdog del loop se apaga mientras tanto: la calibración bloquea)
void atenderMonitorSerie() {
    while (Serial.available() > 0) {
        if (Serial.read() == 'c') {
            ControladorBrujulaSemaforo::borrarCalibracion();
            disableLoopWDT();
            ControladorBrujulaSemaforo::calibrar(CALIBRACION_MANUAL_MS);
            enableLoopWDT();
        }
    }
}

void loop() {
    atenderMonitorSerie();
    unsigned long ahora = millis();

    // ── 1. Máquina de Estados (FSM) del Semáforo ─────────────────────────────
    if (ahora - tiempoInicioEstado >= duracionEstadoActual) {
        // Diagnóstico: si el rumbo cambia varios grados entre una luz y otra,
        // la corriente de los LEDs está desviando la brújula del semáforo
        Serial.printf("[DIAG] Rumbo promedio al terminar la luz: %.1f deg\n",
                      ControladorBrujulaSemaforo::headingActual);
        // Cambiar de estado
        if (luzActual == LuzPeatonal::ROJO) {
            luzActual = LuzPeatonal::VERDE;
            duracionEstadoActual = DURACION_VERDE_MS;
            Serial.println("[SEMAFORO] Luz peatonal: VERDE (puede cruzar)");
        } else {
            luzActual = LuzPeatonal::ROJO;
            duracionEstadoActual = DURACION_ROJO_MS;
            Serial.println("[SEMAFORO] Luz peatonal: ROJO (debe esperar)");
        }
        mostrarLuz(luzActual);
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
        firmarPaqueteSemaforo(&payload); // "ACPI": el brazalete descarta paquetes sin firma
        payload.id = 1; // ID del semáforo en la red local
        payload.grados = ControladorBrujulaSemaforo::gradosParaEnviar();
        payload.permitidoCruce = (luzActual == LuzPeatonal::VERDE) ? 1 : 0;  // verde peatonal = cruzar
        payload.tiempo = tiempoRestante;
        sellarPaqueteSemaforo(&payload); // CRC-8: siempre después de llenar los campos

        // Enviar vía ESP-NOW (broadcast)
        if (ComunicadorBrazalete::enviarEstado(payload)) {
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