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
};

#endif // FSM_H
