// Pruebas de ControladorPatrones: forma de cada patrón, que suene una sola vez
// y la separación mínima de 3 s entre inicios.
#include "minitest.h"
#include "../src/vibracion/controlador_patrones.h"
#include "../src/constantes.h"

namespace {

const SalidaMotores APAGADOS  = {false, false};
const SalidaMotores AMBOS     = {true, true};
const SalidaMotores DERECHO   = {true, false};
const SalidaMotores IZQUIERDO = {false, true};

bool igual(SalidaMotores a, SalidaMotores b) { return a == b; }

} // namespace

PRUEBA(patrones_sin_iniciar_nunca_encienden_motores) {
    ControladorPatrones c;
    VERIFICAR(!c.sonando());
    VERIFICAR(c.libre(0));
    for (uint32_t t = 0; t < 60000; t += 7) {
        VERIFICAR(igual(c.actualizar(t), APAGADOS));
    }
}

PRUEBA(patrones_iniciar_ninguno_se_rechaza) {
    ControladorPatrones c;
    VERIFICAR(!c.iniciar(Patron::NINGUNO, 100));
    VERIFICAR(!c.sonando());
    VERIFICAR(c.libre(100));  // no cuenta como inicio
}

PRUEBA(patrones_duraciones_coinciden_con_patrones_md) {
    VERIFICAR_IGUAL(ControladorPatrones::duracion(Patron::SEMAFORO_DETECTADO), 1600);
    VERIFICAR_IGUAL(ControladorPatrones::duracion(Patron::GIRAR_DERECHA), 300);
    VERIFICAR_IGUAL(ControladorPatrones::duracion(Patron::GIRAR_IZQUIERDA), 300);
    VERIFICAR_IGUAL(ControladorPatrones::duracion(Patron::CRUCE_PERMITIDO), 4500);
    VERIFICAR_IGUAL(ControladorPatrones::duracion(Patron::CRUCE_PROHIBIDO), 2000);
    VERIFICAR_IGUAL(ControladorPatrones::duracion(Patron::NINGUNO), 0);
}

PRUEBA(patrones_semaforo_detectado_tres_pulsos_cortos) {
    ControladorPatrones c;
    const uint32_t t0 = 1000;
    VERIFICAR(c.iniciar(Patron::SEMAFORO_DETECTADO, t0));
    // Pulso 1: 0–199 ms, pausa 200–699, pulso 2: 700–899, pausa, pulso 3: 1400–1599
    VERIFICAR(igual(c.actualizar(t0 + 0),    AMBOS));
    VERIFICAR(igual(c.actualizar(t0 + 199),  AMBOS));
    VERIFICAR(igual(c.actualizar(t0 + 200),  APAGADOS));
    VERIFICAR(igual(c.actualizar(t0 + 699),  APAGADOS));
    VERIFICAR(igual(c.actualizar(t0 + 700),  AMBOS));
    VERIFICAR(igual(c.actualizar(t0 + 899),  AMBOS));
    VERIFICAR(igual(c.actualizar(t0 + 900),  APAGADOS));
    VERIFICAR(igual(c.actualizar(t0 + 1400), AMBOS));
    VERIFICAR(igual(c.actualizar(t0 + 1599), AMBOS));
    VERIFICAR(c.sonando());
    VERIFICAR(igual(c.actualizar(t0 + 1600), APAGADOS));
    VERIFICAR(!c.sonando());  // terminó solo
}

PRUEBA(patrones_contar_pulsos_de_cada_patron) {
    // Recorre cada patrón ms a ms y cuenta los flancos de encendido
    struct Caso { Patron p; int pulsos; SalidaMotores motores; };
    const Caso casos[] = {
        {Patron::SEMAFORO_DETECTADO, 3, AMBOS},
        {Patron::GIRAR_DERECHA,      1, DERECHO},
        {Patron::GIRAR_IZQUIERDA,    1, IZQUIERDO},
        {Patron::CRUCE_PERMITIDO,    2, AMBOS},
        {Patron::CRUCE_PROHIBIDO,    1, AMBOS},
    };
    for (const Caso& caso : casos) {
        ControladorPatrones c;
        c.iniciar(caso.p, 0);
        int flancos = 0;
        SalidaMotores previa = APAGADOS;
        for (uint32_t t = 0; t < 10000; ++t) {
            SalidaMotores s = c.actualizar(t);
            if (!igual(s, APAGADOS)) {
                VERIFICAR(igual(s, caso.motores));  // nunca enciende el motor equivocado
            }
            if (igual(previa, APAGADOS) && !igual(s, APAGADOS)) ++flancos;
            previa = s;
        }
        VERIFICAR_IGUAL(flancos, caso.pulsos);
        VERIFICAR(!c.sonando());
    }
}

PRUEBA(patrones_tiempo_encendido_total) {
    // Tiempo con motores encendidos por patrón: limita el esfuerzo sobre las soldaduras
    struct Caso { Patron p; uint32_t ms; };
    const Caso casos[] = {
        {Patron::SEMAFORO_DETECTADO, 3 * 200},
        {Patron::GIRAR_DERECHA,      300},
        {Patron::CRUCE_PERMITIDO,    2 * 2000},
        {Patron::CRUCE_PROHIBIDO,    2000},
    };
    for (const Caso& caso : casos) {
        ControladorPatrones c;
        c.iniciar(caso.p, 0);
        uint32_t encendido = 0;
        for (uint32_t t = 0; t < 10000; ++t) {
            if (!igual(c.actualizar(t), APAGADOS)) ++encendido;
        }
        VERIFICAR_IGUAL(encendido, caso.ms);
    }
}

PRUEBA(patrones_cruce_permitido_dos_pulsos_de_dos_segundos) {
    ControladorPatrones c;
    c.iniciar(Patron::CRUCE_PERMITIDO, 0);
    VERIFICAR(igual(c.actualizar(0),    AMBOS));
    VERIFICAR(igual(c.actualizar(1999), AMBOS));
    VERIFICAR(igual(c.actualizar(2000), APAGADOS));
    VERIFICAR(igual(c.actualizar(2499), APAGADOS));
    VERIFICAR(igual(c.actualizar(2500), AMBOS));
    VERIFICAR(igual(c.actualizar(4499), AMBOS));
    VERIFICAR(igual(c.actualizar(4500), APAGADOS));
    VERIFICAR(!c.sonando());
}

PRUEBA(patrones_separacion_minima_entre_inicios) {
    ControladorPatrones c;
    VERIFICAR(c.iniciar(Patron::GIRAR_DERECHA, 0));
    c.actualizar(400);  // ya terminó (300 ms)
    VERIFICAR(!c.sonando());
    VERIFICAR(!c.libre(2999));
    VERIFICAR(!c.iniciar(Patron::GIRAR_IZQUIERDA, 2999));
    VERIFICAR(igual(c.actualizar(2999), APAGADOS));  // el rechazo no encendió nada
    VERIFICAR(c.libre(3000));
    VERIFICAR(c.iniciar(Patron::GIRAR_IZQUIERDA, 3000));
}

PRUEBA(patrones_no_se_solapan_aunque_pasen_tres_segundos) {
    ControladorPatrones c;
    c.iniciar(Patron::CRUCE_PERMITIDO, 0);  // dura 4.5 s
    c.actualizar(3500);
    VERIFICAR(c.sonando());
    VERIFICAR(!c.libre(3500));
    VERIFICAR(!c.iniciar(Patron::CRUCE_PROHIBIDO, 3500));
    VERIFICAR_IGUAL(c.patronActual(), Patron::CRUCE_PERMITIDO);
    // Al llegar a su fin queda libre, aunque actualizar() no lo haya visto aún
    VERIFICAR(c.libre(4500));
}

PRUEBA(patrones_cancelar_apaga_pero_no_adelanta_el_siguiente) {
    ControladorPatrones c;
    c.iniciar(Patron::GIRAR_DERECHA, 1000);
    VERIFICAR(igual(c.actualizar(1100), DERECHO));
    c.cancelar();
    VERIFICAR(!c.sonando());
    VERIFICAR(igual(c.actualizar(1150), APAGADOS));
    VERIFICAR(!c.libre(3999));  // la separación se cuenta desde el inicio cancelado
    VERIFICAR(c.libre(4000));
}

PRUEBA(patrones_desborde_de_millis) {
    // millis() se desborda cada ~49 días; las restas sin signo lo absorben
    ControladorPatrones c;
    const uint32_t t0 = 0xFFFFFF00u;  // 256 ms antes del desborde
    VERIFICAR(c.iniciar(Patron::CRUCE_PROHIBIDO, t0));
    VERIFICAR(igual(c.actualizar(t0 + 100), AMBOS));
    VERIFICAR(igual(c.actualizar((uint32_t)(t0 + 1000)), AMBOS));   // ya pasó el desborde
    VERIFICAR(igual(c.actualizar((uint32_t)(t0 + 2000)), APAGADOS));
    VERIFICAR(!c.libre((uint32_t)(t0 + 2999)));
    VERIFICAR(c.libre((uint32_t)(t0 + 3000)));
}
