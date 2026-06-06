#include <Arduino.h>
#include <Wire.h>
#include "../lib/brujula/brujula.h"
#include "../lib/motores/motor.h"
#include "../lib/comunicacion/semaforo/semaforo.h"
#include "../lib/comunicacion/movil/movil.h"
#include "../lib/control/fsm.h"
#include "../include/config.h"

namespace {
    const unsigned long INTERVALO_HEARTBEAT_MS = 2000;
}

unsigned long ultimo_tiempo_heartbeat = 0;

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);
    Serial.println("\n[BOOT] Iniciando Pulsera ACPI v2.0...");

    // 1. Inicializar Motores
    Serial.println("[BOOT] Inicializando motores...");
    if (!ControladorMotores::inicializar()) {
        Serial.println("[BOOT] [ERROR] Fallo al inicializar motores!");
    } else {
        Serial.println("[BOOT] Motores listos.");
    }

    // 2. Inicializar I2C y Sensores (MPU6050 + QMC5883L)
    Serial.println("[BOOT] Inicializando I2C...");
    Wire.begin();
    
    Serial.println("[BOOT] Inicializando brujula...");
    if (!ControladorBrujula::inicializar()) {
        Serial.println("[BOOT] [ERROR] Fallo al inicializar la brújula!");
    } else {
        Serial.println("[BOOT] Brújula lista.");
    }

    // 3. Inicializar Comunicaciones (ESP-NOW + BLE)
    Serial.println("[BOOT] Inicializando receptor ESP-NOW (Semaforo)...");
    if (!ComunicadorSemaforo::inicializar()) {
        Serial.println("[BOOT] [ERROR] Fallo al inicializar receptor ESP-NOW!");
    } else {
        Serial.println("[BOOT] Receptor ESP-NOW listo.");
    }

    Serial.println("[BOOT] Inicializando BLE...");
    if (!ComunicadorMovil::inicializar()) {
        Serial.println("[BOOT] [ERROR] Fallo al inicializar BLE!");
    } else {
        Serial.println("[BOOT] BLE listo.");
    }

    // 4. Inicializar FSM
    Serial.println("[BOOT] Inicializando Maquina de Estados...");
    MaquinaEstados::inicializar();

    Serial.println("[BOOT] Inicializacion completa. Sistema en marcha.");
}

void loop() {
    // Actualizar la Máquina de Estados principal (FSM)
    MaquinaEstados::actualizar();

    // Heartbeat de depuración
    unsigned long ahora = millis();
    if (ahora - ultimo_tiempo_heartbeat >= INTERVALO_HEARTBEAT_MS) {
        ultimo_tiempo_heartbeat = ahora;
        Serial.printf("[SYSTEM] Estado FSM: %d | BLE: %s | Semaforo Angulo: %u\n", 
                      (int)MaquinaEstados::obtenerEstadoActual(),
                      ComunicadorMovil::estaConectado() ? "Conectado" : "Desconectado",
                      ComunicadorSemaforo::ultimoEstado.grados);
    }
}
