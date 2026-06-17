#ifndef FSM_H
#define FSM_H

#include <Arduino.h>
#include <stdint.h>

enum class EstadoPulsera {
    BOOTING,
    STANDBY_LOW_POWER,
    SENSING_CROSSING
};

class MaquinaEstados {
private:
    static EstadoPulsera estadoActual;
    static unsigned long ultimoTiempoPaqueteSemaforo;
    static unsigned long ultimoTiempoEnvioBLE;

public:
    static void inicializar();
    static void registrarPaqueteRecibido();
    static void actualizar();
    static EstadoPulsera obtenerEstadoActual();
    static void setEstado(EstadoPulsera nuevoEstado);
};

#endif // FSM_H
