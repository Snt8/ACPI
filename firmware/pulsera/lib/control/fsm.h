#ifndef FSM_H
#define FSM_H

#include <Arduino.h>
#include <stdint.h>

enum class EstadoPulsera {
    BOOTING,
    STANDBY_LOW_POWER,
    SENSING_CROSSING,
    ALERT_PANIC
};

class MaquinaEstados {
private:
    static EstadoPulsera estadoActual;
    static unsigned long ultimoTiempoPaqueteSemaforo;
    static volatile bool panicoDetectado;
    static unsigned long ultimoTiempoVibracionPanico;
    static bool motorPanicoEncendido;
    static unsigned long ultimoTiempoEnvioBLE;

    // ISR para botón de pánico
    static void IRAM_ATTR manejarInterrupcionPanico();
    
    // Auxiliares de bajo consumo
    static void entrarLightSleep();

public:
    static void inicializar();
    static void registrarPaqueteRecibido();
    static void actualizar();
    static EstadoPulsera obtenerEstadoActual();
    static void setEstado(EstadoPulsera nuevoEstado);
};

#endif // FSM_H
