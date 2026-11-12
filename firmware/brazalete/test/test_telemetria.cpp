// Pruebas del paquete de telemetría BLE (paquete_telemetria.h). El ejemplo de
// "ejemplo_del_protocolo" es el mismo de docs/ble-protocol.md y de las pruebas
// del parser de la app (app-movil/src/protocolo/__tests__/brazalete.test.ts).
#include "minitest.h"
#include "../src/comunicacion/movil/paquete_telemetria.h"
#include <string.h>
#include <string>

namespace {

TelemetriaBle ejemplo() {
    TelemetriaBle t;
    t.secuencia = 412;
    t.rumbo = 21;
    t.estado = 1;
    t.orientado = true;
    t.tiempo = 18;
    t.bateria = -1;
    t.semaforo = 1;
    t.verde = true;
    t.rumbo_semaforo = 200;
    t.giro = -1;
    t.brujula = EstadoBrujulaBle::VALIDA;
    t.patron = 4;
    t.recibidos = 20;
    t.rssi = -67;
    return t;
}

} // namespace

PRUEBA(telemetria_ejemplo_del_protocolo) {
    char buf[200];
    size_t largo = formatearTelemetria(ejemplo(), buf, sizeof(buf));
    const char* esperado =
        "{\"v\":2,\"n\":412,\"hd\":21,\"st\":1,\"ok\":true,\"tr\":18,\"bt\":-1,"
        "\"id\":1,\"lz\":1,\"hs\":200,\"gr\":-1,\"br\":1,\"pt\":4,\"rx\":20,\"rs\":-67}";
    VERIFICAR(std::string(buf) == esperado);
    VERIFICAR_IGUAL(largo, strlen(esperado));
}

PRUEBA(telemetria_conserva_los_campos_de_la_version_1) {
    // Una app que sólo conoce hd, st, ok, tr y bt los encuentra igual
    char buf[200];
    formatearTelemetria(ejemplo(), buf, sizeof(buf));
    std::string s(buf);
    VERIFICAR(s.find("\"hd\":21,\"st\":1,\"ok\":true,\"tr\":18,\"bt\":-1") != std::string::npos);
}

PRUEBA(telemetria_el_peor_caso_cabe_en_una_notificacion) {
    TelemetriaBle t;
    t.secuencia = 65535; t.rumbo = 359; t.estado = 255; t.orientado = false; t.tiempo = 255;
    t.bateria = -128; t.semaforo = 255; t.verde = false; t.rumbo_semaforo = 359; t.giro = -180;
    t.brujula = (EstadoBrujulaBle)255; t.patron = 255; t.recibidos = 255; t.rssi = -128;
    char buf[300];
    size_t largo = formatearTelemetria(t, buf, sizeof(buf));
    VERIFICAR(largo > 0);
    VERIFICAR(largo <= TELEMETRIA_LARGO_MAXIMO);
    VERIFICAR(TELEMETRIA_LARGO_MAXIMO <= 185 - 3);  // MTU que pide el brazalete
}

PRUEBA(telemetria_rumbos_siempre_bajo_360) {
    TelemetriaBle t = ejemplo();
    t.rumbo = 360;
    t.rumbo_semaforo = 725;
    char buf[200];
    formatearTelemetria(t, buf, sizeof(buf));
    std::string s(buf);
    VERIFICAR(s.find("\"hd\":0,") != std::string::npos);
    VERIFICAR(s.find("\"hs\":5,") != std::string::npos);
}

PRUEBA(telemetria_sin_espacio_no_escribe_medio_json) {
    char buf[40];
    VERIFICAR_IGUAL(formatearTelemetria(ejemplo(), buf, sizeof(buf)), (size_t)0);
    VERIFICAR_IGUAL(buf[0], '\0');
}
