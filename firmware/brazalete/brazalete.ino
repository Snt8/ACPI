// Firmware del brazalete ACPI (ESP32-C3 Super Mini).
// Arduino IDE: abrir este archivo; el resto del código está en src/ y se compila solo.
// ESP-IDF: main/main.cpp incluye este archivo (ver README.md).
#include <Arduino.h>
#include <Wire.h>
#include "src/brujula/brujula.h"
#include "src/motores/motor.h"
#include "src/comunicacion/semaforo/semaforo.h"
#include "src/comunicacion/movil/movil.h"
#include "src/control/fsm.h"
#include "src/config.h"

namespace {
    const unsigned long INTERVALO_HEARTBEAT_MS = 2000;
}

unsigned long ultimo_tiempo_heartbeat = 0;

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);
    Serial.println("\n[BOOT] Iniciando Brazalete ACPI v2.0...");

    // 1. Inicializar Motores
    Serial.println("[BOOT] Inicializando motores...");
    if (!ControladorMotores::inicializar()) {
        Serial.println("[BOOT] [ERROR] Fallo al inicializar motores!");
    } else {
        Serial.println("[BOOT] Motores listos.");
    }

    // 2. Inicializar I2C y Sensores (MPU6050 + QMC5883L) — pines en config.h
    Serial.println("[BOOT] Inicializando I2C...");
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    
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
