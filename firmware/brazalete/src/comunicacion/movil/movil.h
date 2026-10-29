#ifndef COMUNICACION_MOVIL_H
#define COMUNICACION_MOVIL_H

#include <Arduino.h>
#include <stdint.h>
#include "paquete_telemetria.h"

// Comandos que la app escribe en la característica RX (docs/ble-protocol.md)
enum class ComandoMovil : uint8_t {
    NINGUNO,
    PROBAR_VIBRACION,   // "TEST_ALERT"
    DETENER_VIBRACION   // "VIBRATE_STOP"
};

class ComunicadorMovil {
public:
    static bool inicializar();
    static bool estaConectado();
    // Notifica el paquete de telemetría (paquete_telemetria.h) si hay un móvil conectado
    static void enviarTelemetria(const TelemetriaBle& telemetria);
    // El último comando recibido sin atender (lo escribe la tarea de BLE y lo
    // atiende loop(), para no tocar los motores desde otro hilo)
    static ComandoMovil tomarComando();
    static void desinicializar();
};

#endif // COMUNICACION_MOVIL_H
