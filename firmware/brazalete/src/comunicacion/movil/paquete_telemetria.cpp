#include "paquete_telemetria.h"
#include <stdio.h>

size_t formatearTelemetria(const TelemetriaBle& t, char* destino, size_t capacidad) {
    // Mismo orden que docs/ble-protocol.md: primero la versión, después los
    // campos de la versión 1 y al final los nuevos
    int largo = snprintf(destino, capacidad,
                         "{\"v\":%d,\"n\":%u,\"hd\":%u,\"st\":%u,\"ok\":%s,\"tr\":%u,\"bt\":%d,"
                         "\"id\":%u,\"lz\":%u,\"hs\":%u,\"gr\":%d,\"br\":%u,\"pt\":%u,\"rx\":%u,\"rs\":%d}",
                         TELEMETRIA_VERSION, (unsigned)t.secuencia, (unsigned)(t.rumbo % 360),
                         (unsigned)t.estado, t.orientado ? "true" : "false", (unsigned)t.tiempo,
                         (int)t.bateria, (unsigned)t.semaforo, t.verde ? 1u : 0u,
                         (unsigned)(t.rumbo_semaforo % 360), (int)t.giro, (unsigned)t.brujula,
                         (unsigned)t.patron, (unsigned)t.recibidos, (int)t.rssi);
    if (largo < 0 || (size_t)largo >= capacidad) {
        if (capacidad > 0) destino[0] = '\0';
        return 0;
    }
    return (size_t)largo;
}
