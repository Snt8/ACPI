#ifndef FSM_H
#define FSM_H

#include <Arduino.h>
#include <stdint.h>
#include "../protocolo/protocolo_esp.h"

enum class EstadoBrazalete {
    BOOTING,
    STANDBY_LOW_POWER,
    SENSING_CROSSING
};

class MaquinaEstados {
private:
    static EstadoBrazalete estadoActual;
    // Hace la transición y enciende/apaga lo que corresponde a cada estado
    static void setEstado(EstadoBrazalete nuevoEstado);

public:
    static void inicializar();
    static void actualizar();
    static EstadoBrazalete obtenerEstadoActual();
    // Último paquete del semáforo que se está siguiendo (vacío en standby)
    static const estado_semaforo& ultimoPaquete();

    // Vuelve a standby con un reinicio limpio del ESP32 (motores apagados).
    // Apagar y volver a encender el BLE y el WiFi en caliente es frágil en el
    // core de Arduino; el arranque, en cambio, siempre encuentra al semáforo.
    // Tras este reinicio no se calibra aunque falte la calibración.
    static void reiniciarEnStandby(const char* motivo);
    // true si este arranque lo provocó reiniciarEnStandby() (y lo olvida).
    // Llamarla una vez, al principio de setup().
    static bool arranqueTrasStandby();
};

#endif // FSM_H
