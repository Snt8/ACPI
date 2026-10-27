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
    static unsigned long ultimoTiempoPaqueteSemaforo;
    static unsigned long ultimoTiempoEnvioBLE;

public:
    static void inicializar();
    static void actualizar();
    static EstadoBrazalete obtenerEstadoActual();
    // Último paquete del semáforo que se está siguiendo (vacío en standby)
    static const estado_semaforo& ultimoPaquete();
    static void setEstado(EstadoBrazalete nuevoEstado);
};

#endif // FSM_H
