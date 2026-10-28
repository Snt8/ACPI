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
#include "src/constantes.h"

namespace {
    const unsigned long INTERVALO_HEARTBEAT_MS = 2000;

    // Por qué se reinició el ESP32 la última vez. Sirve para diagnosticar
    // reinicios en bucle: BROWNOUT = cayó el voltaje (p. ej. una pista de
    // alimentación que hace contacto a ratos), WATCHDOG = loop() se quedó pegado.
    const char* motivoReinicio() {
        switch (esp_reset_reason()) {
            case ESP_RST_POWERON:   return "ENCENDIDO (alimentacion conectada)";
            case ESP_RST_EXT:       return "PIN EXTERNO";
            case ESP_RST_SW:        return "SOFTWARE (carga de programa o esp_restart)";
            case ESP_RST_PANIC:     return "PANICO (fallo del programa)";
            case ESP_RST_INT_WDT:
            case ESP_RST_TASK_WDT:
            case ESP_RST_WDT:       return "WATCHDOG (loop() se quedo pegado)";
            case ESP_RST_DEEPSLEEP: return "SALIDA DE DEEP SLEEP";
            case ESP_RST_BROWNOUT:  return "BROWNOUT (cayo el voltaje de alimentacion)";
            case ESP_RST_USB:       return "USB (boton RESET o reconexion)";
            default:                return "DESCONOCIDO";
        }
    }
}

unsigned long ultimo_tiempo_heartbeat = 0;
uint32_t descartados_anteriores = 0;

void setup() {
    // 1. Motores apagados ANTES que nada: la PCB v3.5 no tiene resistencia de
    //    base a GND en los transistores, así que desde el reset hasta aquí la
    //    base queda flotando y un motor podría dar un tirón. Cuanto antes se
    //    fijen los pines en 0, más corto es ese intervalo.
    bool motores_ok = ControladorMotores::inicializar();

    Serial.begin(BAUD_RATE);
    delay(500);
    Serial.println("\n[BOOT] Iniciando Brazalete ACPI v2.0...");
    Serial.printf("[BOOT] Motivo del ultimo reinicio: %s\n", motivoReinicio());
    Serial.println(motores_ok ? "[BOOT] Motores listos (apagados)."
                              : "[BOOT] [ERROR] Fallo al inicializar motores!");

    // 2. Inicializar I2C y Sensores (MPU6050 + QMC5883L) — pines en config.h
    Serial.println("[BOOT] Inicializando I2C...");
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    
    Serial.println("[BOOT] Inicializando brujula...");
    if (!ControladorBrujula::inicializar()) {
        Serial.println("[BOOT] [ERROR] Fallo al inicializar la brújula!");
    } else {
        Serial.println("[BOOT] Brújula lista.");
    }

    // 3. Las radios (ESP-NOW y BLE) no se encienden aquí: el brazalete arranca
    //    en standby, que escucha al semáforo cada 5 s, y el BLE se enciende
    //    recién al encontrarlo (ver src/control/modo_standby.h)

    // 4. Inicializar FSM
    Serial.println("[BOOT] Inicializando Maquina de Estados...");
    MaquinaEstados::inicializar();

    // 5. Watchdog del loop: si loop() deja de volver por más de
    //    TIMEOUT_WATCHDOG_LOOP_MS (algo se quedó pegado), el ESP32 se reinicia
    //    y arranca de nuevo en standby con los motores apagados. El core lo
    //    alimenta solo al final de cada vuelta de loop().
    enableLoopWDT();

    Serial.println("[BOOT] Inicializacion completa. Sistema en marcha.");
}

void loop() {
    // Actualizar la Máquina de Estados principal (FSM)
    MaquinaEstados::actualizar();

    // Heartbeat de depuración
    unsigned long ahora = millis();
    if (ahora - ultimo_tiempo_heartbeat >= INTERVALO_HEARTBEAT_MS) {
        ultimo_tiempo_heartbeat = ahora;
        // El heap libre permite ver si los ciclos de standby pierden memoria
        Serial.printf("[SYSTEM] Estado FSM: %d | BLE: %s | Semaforo Angulo: %u | "
                      "ESP-NOW descartados: %lu | Heap libre: %lu\n",
                      (int)MaquinaEstados::obtenerEstadoActual(),
                      ComunicadorMovil::estaConectado() ? "Conectado" : "Desconectado",
                      MaquinaEstados::ultimoPaquete().grados,
                      (unsigned long)ComunicadorSemaforo::paquetesDescartados(),
                      (unsigned long)ESP.getFreeHeap());

        // Muchos paquetes ESP-NOW ajenos: sólo se avisa, ya se descartan solos
        uint32_t descartados = ComunicadorSemaforo::paquetesDescartados();
        if (descartados - descartados_anteriores > UMBRAL_INTERFERENCIA_PAQUETES) {
            Serial.printf("[RADIO] Interferencia: %lu paquetes ajenos en %lu ms (ignorados)\n",
                          (unsigned long)(descartados - descartados_anteriores),
                          (unsigned long)INTERVALO_HEARTBEAT_MS);
        }
        descartados_anteriores = descartados;
    }
}
