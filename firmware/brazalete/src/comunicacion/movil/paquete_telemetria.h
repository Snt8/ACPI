#ifndef PAQUETE_TELEMETRIA_H
#define PAQUETE_TELEMETRIA_H

#include <stddef.h>
#include <stdint.h>

// ── Paquete de telemetría BLE (brazalete → app), versión 2 ───────────────────
// JSON compacto en ASCII que se notifica cada INTERVALO_TELEMETRIA_BLE_MS por
// la característica TX. El contrato completo está en docs/ble-protocol.md y el
// parser de la app en app-movil/src/protocolo/brazalete.ts: si se cambia un
// campo aquí, hay que cambiarlo allá.
//
// Los campos de la versión 1 (hd, st, ok, tr, bt) no cambian de nombre ni de
// significado, así una app vieja sigue funcionando con este firmware.
//
// No usa Arduino: se prueba en la PC (test/).

const int TELEMETRIA_VERSION = 2;

// Lo que mide el brazalete para el paquete. Las abreviaturas son las claves JSON.
enum class EstadoBrujulaBle : uint8_t {
    SIN_CALIBRAR = 0,  // no indica orientación hasta calibrar (ver brujula/README.md)
    VALIDA       = 1,  // la última lectura sirve
    DESCARTADA   = 2   // la última lectura se descartó (muñeca en movimiento, giro brusco, sensor)
};

struct TelemetriaBle {
    uint16_t secuencia;       // n:  sube 1 por paquete (vuelve a 0 tras 65535): detecta perdidos
    uint16_t rumbo;           // hd: rumbo del brazalete, 0-359
    uint8_t  estado;          // st: 0 rojo de frente, 1 verde de frente, 2 mal orientado, 3 pánico
    bool     orientado;       // ok: de frente al cruce
    uint8_t  tiempo;          // tr: segundos que le quedan a la luz actual
    int8_t   bateria;         // bt: %, o -1 si no se mide
    uint8_t  semaforo;        // id: id del semáforo que se sigue
    bool     verde;           // lz: luz peatonal (1 verde / 0 rojo), aunque esté mal orientada
    uint16_t rumbo_semaforo;  // hs: rumbo que envía el semáforo, 0-359
    int16_t  giro;            // gr: grados para quedar de frente, -180..180 (+ derecha)
    EstadoBrujulaBle brujula; // br
    uint8_t  patron;          // pt: patrón que vibra ahora (valor de Patron; 0 = ninguno)
    uint8_t  recibidos;       // rx: paquetes del semáforo en los últimos 10 s (llegan 2 por s)
    int8_t   rssi;            // rs: señal del último paquete del semáforo, dBm (0 = sin dato)
};

// Largo máximo del JSON con los peores valores de cada campo. Tiene que caber
// en una notificación: MTU − 3 bytes (el brazalete pide MTU_BLE = 185).
const size_t TELEMETRIA_LARGO_MAXIMO = 160;

// Escribe el JSON en 'destino' (con '\0'). Devuelve su largo, o 0 si no cabe.
size_t formatearTelemetria(const TelemetriaBle& t, char* destino, size_t capacidad);

#endif // PAQUETE_TELEMETRIA_H
