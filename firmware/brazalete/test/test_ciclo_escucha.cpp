// Pruebas de CicloEscucha: en standby la radio sólo se enciende una ventana
// corta cada 5 s, y el resto del tiempo está apagada.
#include "minitest.h"
#include "../src/control/ciclo_escucha.h"
#include "../src/constantes.h"

using Accion = CicloEscucha::Accion;

PRUEBA(escucha_al_arrancar_escucha_de_inmediato) {
    CicloEscucha c;
    c.iniciar(1000, true);
    VERIFICAR(!c.escuchando());
    VERIFICAR_IGUAL(c.msHastaProximaEscucha(1000), 0);
    VERIFICAR_IGUAL(c.actualizar(1000), Accion::ENCENDER_RADIO);
    VERIFICAR(c.escuchando());
}

PRUEBA(escucha_al_entrar_desde_sensing_espera_un_intervalo) {
    CicloEscucha c;
    c.iniciar(1000, false);
    VERIFICAR_IGUAL(c.msHastaProximaEscucha(1000), INTERVALO_ESCUCHA_STANDBY_MS);
    VERIFICAR_IGUAL(c.actualizar(1000 + INTERVALO_ESCUCHA_STANDBY_MS - 1), Accion::NADA);
    VERIFICAR_IGUAL(c.actualizar(1000 + INTERVALO_ESCUCHA_STANDBY_MS), Accion::ENCENDER_RADIO);
}

PRUEBA(escucha_ventana_y_periodo) {
    CicloEscucha c;
    c.iniciar(0, true);
    VERIFICAR_IGUAL(c.actualizar(0), Accion::ENCENDER_RADIO);
    VERIFICAR_IGUAL(c.actualizar(VENTANA_ESCUCHA_STANDBY_MS - 1), Accion::NADA);
    VERIFICAR(c.escuchando());
    VERIFICAR_IGUAL(c.actualizar(VENTANA_ESCUCHA_STANDBY_MS), Accion::APAGAR_RADIO);
    VERIFICAR(!c.escuchando());
    VERIFICAR_IGUAL(c.msHastaProximaEscucha(VENTANA_ESCUCHA_STANDBY_MS),
                    INTERVALO_ESCUCHA_STANDBY_MS - VENTANA_ESCUCHA_STANDBY_MS);
    VERIFICAR_IGUAL(c.actualizar(INTERVALO_ESCUCHA_STANDBY_MS - 1), Accion::NADA);
    VERIFICAR_IGUAL(c.actualizar(INTERVALO_ESCUCHA_STANDBY_MS), Accion::ENCENDER_RADIO);
}

PRUEBA(escucha_radio_encendida_solo_una_fraccion_del_tiempo) {
    // Un minuto ms a ms: la radio está encendida VENTANA de cada INTERVALO
    CicloEscucha c;
    c.iniciar(0, true);
    uint32_t encendida = 0;
    int encendidos = 0, apagados = 0;
    for (uint32_t t = 0; t < 60000; ++t) {
        Accion a = c.actualizar(t);
        encendidos += (a == Accion::ENCENDER_RADIO);
        apagados   += (a == Accion::APAGAR_RADIO);
        if (c.escuchando()) ++encendida;
    }
    VERIFICAR_IGUAL(encendidos, 60000 / INTERVALO_ESCUCHA_STANDBY_MS);
    VERIFICAR_IGUAL(apagados, encendidos);
    VERIFICAR_IGUAL(encendida, encendidos * VENTANA_ESCUCHA_STANDBY_MS);
}

PRUEBA(escucha_despertar_tarde_no_encadena_ventanas) {
    // Si el loop vuelve tarde (por ejemplo, tras un light sleep largo), la
    // ventana empieza en ese momento y la siguiente se cuenta desde ahí
    CicloEscucha c;
    c.iniciar(0, false);
    const uint32_t tarde = INTERVALO_ESCUCHA_STANDBY_MS * 3 + 777;
    VERIFICAR_IGUAL(c.actualizar(tarde), Accion::ENCENDER_RADIO);
    VERIFICAR_IGUAL(c.actualizar(tarde + VENTANA_ESCUCHA_STANDBY_MS), Accion::APAGAR_RADIO);
    VERIFICAR_IGUAL(c.actualizar(tarde + VENTANA_ESCUCHA_STANDBY_MS + 1), Accion::NADA);
    VERIFICAR_IGUAL(c.msHastaProximaEscucha(tarde + VENTANA_ESCUCHA_STANDBY_MS),
                    INTERVALO_ESCUCHA_STANDBY_MS - VENTANA_ESCUCHA_STANDBY_MS);
}

PRUEBA(escucha_desborde_de_millis) {
    CicloEscucha c;
    const uint32_t t0 = 0xFFFFFFFFu - 1000u;
    c.iniciar(t0, false);
    VERIFICAR(c.msHastaProximaEscucha(t0) == INTERVALO_ESCUCHA_STANDBY_MS);
    VERIFICAR_IGUAL(c.actualizar((uint32_t)(t0 + 2000)), Accion::NADA);  // ya pasó el desborde
    VERIFICAR_IGUAL(c.actualizar((uint32_t)(t0 + INTERVALO_ESCUCHA_STANDBY_MS)),
                    Accion::ENCENDER_RADIO);
}

PRUEBA(escucha_ventana_capta_al_menos_dos_paquetes) {
    // El semáforo envía cada INTERVALO_ENVIO_SEMAFORO_MS: la ventana debe
    // abarcar dos envíos aunque el WiFi tarde en arrancar
    VERIFICAR(VENTANA_ESCUCHA_STANDBY_MS > 2 * INTERVALO_ENVIO_SEMAFORO_MS);
    VERIFICAR(VENTANA_ESCUCHA_STANDBY_MS < INTERVALO_ESCUCHA_STANDBY_MS);
}
