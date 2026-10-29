// Pruebas de EvaluadorSemaforo: qué diferencia entre paquetes es un evento,
// y qué patrón pide según la orientación de la persona.
#include "minitest.h"
#include "../src/control/evaluador_semaforo.h"
#include "../src/config.h"
#include "../src/constantes.h"

namespace {

estado_semaforo paquete(uint8_t id, uint16_t grados, uint8_t permitido, uint8_t tiempo = 10) {
    estado_semaforo p{};
    firmarPaqueteSemaforo(&p);
    p.id = id;
    p.grados = grados;
    p.permitidoCruce = permitido;
    p.tiempo = tiempo;
    return p;
}

Orientacion deFrente()              { return {true, 2.0f, 2.0f}; }
Orientacion malOrientada(float giro) { return {true, 90.0f, giro}; }
Orientacion conDesviacion(float d)   { return {true, d, d}; }
// Una lectura inválida "dice" estar de frente: si el evaluador la usara por
// error, cambiaría de estado y las pruebas lo notarían
Orientacion invalida()               { return {false, 0.0f, 0.0f}; }

// Las lecturas seguidas que hacen falta para quedar "de frente"
void orientarDeFrente(EvaluadorSemaforo& e) {
    for (int i = 0; i < CONFIRMACIONES_ORIENTACION; ++i) e.registrarOrientacion(deFrente());
}

// Avanza el controlador hasta que termine el patrón en curso
void dejarTerminar(ControladorPatrones& c, uint32_t& t) {
    while (c.sonando()) c.actualizar(++t);
}

// Semáforo detectado, persona de frente y luz ya avisada (la histéresis sólo
// protege un aviso ya dado)
void deFrenteConLuzAvisada(EvaluadorSemaforo& e, ControladorPatrones& c, uint32_t& t,
                           float desviacion = 2.0f) {
    e.registrarPaquete(paquete(1, 90, 1));
    e.orquestar(t);  // detectado
    dejarTerminar(c, t);
    for (int i = 0; i < CONFIRMACIONES_ORIENTACION; ++i) {
        e.registrarOrientacion({true, desviacion, desviacion});
    }
    t += SEPARACION_MINIMA_EVENTOS_MS;
    e.orquestar(t);  // aviso de la luz
    dejarTerminar(c, t);
}

} // namespace

// ── Diferencias entre paquetes ───────────────────────────────────────────────

PRUEBA(evaluador_primer_paquete_es_semaforo_nuevo) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    VERIFICAR(!e.haySemaforo());
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 0)), CambioPaquete::SEMAFORO_NUEVO);
    VERIFICAR(e.haySemaforo());
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 0)), CambioPaquete::SIN_CAMBIOS);
}

PRUEBA(evaluador_grados_y_tiempo_no_son_eventos) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 1, 20));
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 95, 1, 19)), CambioPaquete::SIN_CAMBIOS);
    VERIFICAR_IGUAL(e.ultimoPaquete().grados, 95);  // pero sí se guardan
    VERIFICAR_IGUAL(e.ultimoPaquete().tiempo, 19);
}

PRUEBA(evaluador_cambio_de_luz) {
    // El cambio cuenta al segundo paquete seguido con la luz nueva
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 0));
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 1)), CambioPaquete::CAMBIO_POR_CONFIRMAR);
    VERIFICAR_IGUAL(e.ultimoPaquete().permitidoCruce, 0);  // todavía vale la luz vieja
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 1)), CambioPaquete::CAMBIO_A_PERMITIDO);
    VERIFICAR_IGUAL(e.ultimoPaquete().permitidoCruce, 1);
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 1)), CambioPaquete::SIN_CAMBIOS);
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 0)), CambioPaquete::CAMBIO_POR_CONFIRMAR);
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 0)), CambioPaquete::CAMBIO_A_PROHIBIDO);
}

PRUEBA(evaluador_paquete_suelto_con_otra_luz_no_cambia_nada) {
    // Interferencia u otro equipo: un paquete aislado con la luz contraria entre
    // paquetes normales no es un cambio de luz y no hace vibrar
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 1));
    orientarDeFrente(e);
    e.orquestar(t);
    dejarTerminar(c, t);
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::CRUCE_PERMITIDO);
    t = 3000;
    dejarTerminar(c, t);
    for (int i = 0; i < 20; ++i) {
        VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 1)), CambioPaquete::SIN_CAMBIOS);
        VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 0)), CambioPaquete::CAMBIO_POR_CONFIRMAR);
    }
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 1)), CambioPaquete::SIN_CAMBIOS);
    VERIFICAR(e.cruceNotificado());
    VERIFICAR_IGUAL(e.ultimoPaquete().permitidoCruce, 1);
    VERIFICAR_IGUAL(e.orquestar(20000), Patron::NINGUNO);
}

PRUEBA(evaluador_cambio_sin_confirmar_actualiza_solo_los_grados) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 0, 20));
    e.registrarPaquete(paquete(1, 95, 1, 25));
    VERIFICAR_IGUAL(e.ultimoPaquete().grados, 95);
    VERIFICAR_IGUAL(e.ultimoPaquete().permitidoCruce, 0);
    VERIFICAR_IGUAL(e.ultimoPaquete().tiempo, 20);  // el tiempo es de la luz vigente
}

PRUEBA(evaluador_permitido_distinto_de_cero_cuenta_como_uno) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 1));
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 2)), CambioPaquete::SIN_CAMBIOS);
}

PRUEBA(evaluador_ignora_otro_semaforo) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 0));
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(2, 270, 1)), CambioPaquete::IGNORADO);
    VERIFICAR_IGUAL(e.ultimoPaquete().id, 1);
    VERIFICAR_IGUAL(e.ultimoPaquete().permitidoCruce, 0);
    // Tras reiniciar (standby), el siguiente semáforo que se escuche es "nuevo"
    e.reiniciar();
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(2, 270, 1)), CambioPaquete::SEMAFORO_NUEVO);
}

// ── Orquestación de patrones ─────────────────────────────────────────────────

PRUEBA(evaluador_sin_semaforo_no_vibra) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarOrientacion(malOrientada(90));
    for (uint32_t t = 0; t < 60000; t += 10) {
        VERIFICAR_IGUAL(e.orquestar(t), Patron::NINGUNO);
    }
}

PRUEBA(evaluador_detectado_suena_una_sola_vez) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 0));
    VERIFICAR_IGUAL(e.orquestar(0), Patron::SEMAFORO_DETECTADO);
    // Sin orientación válida no se pide nada más, por mucho que lleguen paquetes
    for (uint32_t t = 10; t < 60000; t += 10) {
        e.registrarPaquete(paquete(1, 90, 0));
        c.actualizar(t);
        VERIFICAR_IGUAL(e.orquestar(t), Patron::NINGUNO);
    }
}

PRUEBA(evaluador_mal_orientada_pulso_cada_tres_segundos) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 0));
    VERIFICAR_IGUAL(e.orquestar(t), Patron::SEMAFORO_DETECTADO);
    dejarTerminar(c, t);

    e.registrarOrientacion(malOrientada(+50.0f));
    t = 3000;  // separación mínima desde el aviso de detección
    VERIFICAR_IGUAL(e.orquestar(t), Patron::GIRAR_DERECHA);
    dejarTerminar(c, t);
    VERIFICAR_IGUAL(e.orquestar(5999), Patron::NINGUNO);
    VERIFICAR_IGUAL(e.orquestar(6000), Patron::GIRAR_DERECHA);
    t = 6000;
    dejarTerminar(c, t);

    e.registrarOrientacion(malOrientada(-50.0f));
    VERIFICAR_IGUAL(e.orquestar(8999), Patron::NINGUNO);
    VERIFICAR_IGUAL(e.orquestar(9000), Patron::GIRAR_IZQUIERDA);
}

PRUEBA(evaluador_de_frente_avisa_la_luz_una_vez) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 1));
    orientarDeFrente(e);
    VERIFICAR_IGUAL(e.orquestar(t), Patron::SEMAFORO_DETECTADO);
    dejarTerminar(c, t);
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::CRUCE_PERMITIDO);
    VERIFICAR(e.cruceNotificado());
    t = 3000;
    // Persona quieta y misma luz durante un minuto: silencio absoluto
    for (t += 10; t < 63000; t += 10) {
        e.registrarPaquete(paquete(1, 90, 1));
        orientarDeFrente(e);
        c.actualizar(t);
        VERIFICAR_IGUAL(e.orquestar(t), Patron::NINGUNO);
    }
}

PRUEBA(evaluador_cambio_de_luz_de_frente_vibra_una_vez) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 0));
    orientarDeFrente(e);
    e.orquestar(t);  // detectado
    dejarTerminar(c, t);
    t = 3000;
    VERIFICAR_IGUAL(e.orquestar(t), Patron::CRUCE_PROHIBIDO);
    dejarTerminar(c, t);

    t = 20000;
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 1)), CambioPaquete::CAMBIO_POR_CONFIRMAR);
    VERIFICAR_IGUAL(e.orquestar(t), Patron::NINGUNO);  // sin confirmar no vibra
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 1)), CambioPaquete::CAMBIO_A_PERMITIDO);
    VERIFICAR_IGUAL(e.orquestar(t), Patron::CRUCE_PERMITIDO);
    dejarTerminar(c, t);
    VERIFICAR_IGUAL(e.orquestar(t + 10000), Patron::NINGUNO);
}

PRUEBA(evaluador_cambio_de_luz_mal_orientada_espera_a_quedar_de_frente) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 0));
    e.registrarOrientacion(malOrientada(60));
    e.orquestar(t);  // detectado
    dejarTerminar(c, t);
    e.registrarPaquete(paquete(1, 90, 1));
    e.registrarPaquete(paquete(1, 90, 1));  // cambio confirmado
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::GIRAR_DERECHA);  // primero orientarse
    t = 3000;
    dejarTerminar(c, t);
    orientarDeFrente(e);
    VERIFICAR_IGUAL(e.orquestar(6000), Patron::CRUCE_PERMITIDO);  // la luz actual
}

PRUEBA(evaluador_histeresis_de_orientacion) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    deFrenteConLuzAvisada(e, c, t, UMBRAL_CRUCE_GRADOS - 1);
    VERIFICAR(e.orientado());
    VERIFICAR(e.cruceNotificado());
    // Entre el umbral y umbral + histéresis sigue "de frente"
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS + HISTERESIS_ORIENTACION_GRADOS / 2));
    VERIFICAR(e.orientado());
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS + HISTERESIS_ORIENTACION_GRADOS));
    VERIFICAR(e.orientado());
    // Una sola lectura fuera de la banda basta para volver a "mal orientada"
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS + HISTERESIS_ORIENTACION_GRADOS + 0.5f));
    VERIFICAR(!e.orientado());
    // Para volver a "de frente" hay que bajar del umbral, no de umbral + histéresis
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS + HISTERESIS_ORIENTACION_GRADOS / 2));
    VERIFICAR(!e.orientado());
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS));
    VERIFICAR(!e.orientado());  // el umbral es estricto (<)
    for (int i = 0; i < CONFIRMACIONES_ORIENTACION; ++i) {
        e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS - 0.5f));
    }
    VERIFICAR(e.orientado());
}

PRUEBA(evaluador_umbral_de_cruce_es_cinco_grados) {
    // Pedido el 28 oct 2026: sólo se autoriza el cruce a menos de 5° del eje
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    for (int i = 0; i < 10; ++i) e.registrarOrientacion(conDesviacion(5.5f));
    VERIFICAR(!e.orientado());
    uint32_t t = 0;
    deFrenteConLuzAvisada(e, c, t, 4.5f);
    VERIFICAR(e.orientado());
    e.registrarOrientacion(conDesviacion(9.5f));
    VERIFICAR(e.orientado());
    e.registrarOrientacion(conDesviacion(10.5f));
    VERIFICAR(!e.orientado());
}

PRUEBA(evaluador_de_frente_exige_lecturas_seguidas) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    // Una lectura suelta dentro del cono (la muñeca pasando mientras gira) no basta
    e.registrarOrientacion(deFrente());
    if (CONFIRMACIONES_ORIENTACION > 1) VERIFICAR(!e.orientado());
    e.registrarOrientacion(malOrientada(20));
    e.registrarOrientacion(deFrente());
    if (CONFIRMACIONES_ORIENTACION > 1) VERIFICAR(!e.orientado());
    // Una lectura inválida también corta la racha
    e.registrarOrientacion(invalida());
    e.registrarOrientacion(deFrente());
    if (CONFIRMACIONES_ORIENTACION > 1) VERIFICAR(!e.orientado());
    for (int i = 1; i < CONFIRMACIONES_ORIENTACION; ++i) e.registrarOrientacion(deFrente());
    VERIFICAR(e.orientado());
}

PRUEBA(evaluador_la_orientacion_va_antes_que_la_luz) {
    // Con el semáforo detectado y la luz en verde, la persona pasa por el cono
    // con una sola lectura y sigue de largo: no se avisa la luz, se indica el
    // giro. La luz suena sólo con la orientación confirmada.
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 1));
    VERIFICAR_IGUAL(e.orquestar(t), Patron::SEMAFORO_DETECTADO);
    dejarTerminar(c, t);
    e.registrarOrientacion(deFrente());
    if (CONFIRMACIONES_ORIENTACION > 1) {
        // Ni luz ni giro mientras se confirma
        VERIFICAR_IGUAL(e.orquestar(3000), Patron::NINGUNO);
        e.registrarOrientacion(malOrientada(-30));
        VERIFICAR_IGUAL(e.orquestar(3000), Patron::GIRAR_IZQUIERDA);
        t = 3000;
        dejarTerminar(c, t);
        orientarDeFrente(e);
        VERIFICAR_IGUAL(e.orquestar(6000), Patron::CRUCE_PERMITIDO);
    } else {
        VERIFICAR_IGUAL(e.orquestar(3000), Patron::CRUCE_PERMITIDO);
    }
}

PRUEBA(evaluador_volver_a_quedar_de_frente_repite_la_luz) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 0));
    orientarDeFrente(e);
    e.orquestar(t);
    dejarTerminar(c, t);
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::CRUCE_PROHIBIDO);
    t = 3000;
    dejarTerminar(c, t);
    e.registrarOrientacion(malOrientada(-70));
    VERIFICAR(!e.cruceNotificado());
    VERIFICAR_IGUAL(e.orquestar(6000), Patron::GIRAR_IZQUIERDA);
    t = 6000;
    dejarTerminar(c, t);
    orientarDeFrente(e);
    VERIFICAR_IGUAL(e.orquestar(8999), Patron::NINGUNO);  // separación mínima
    VERIFICAR_IGUAL(e.orquestar(9000), Patron::CRUCE_PROHIBIDO);
}

PRUEBA(evaluador_lectura_invalida_no_indica_giro) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 0));
    e.orquestar(t);  // detectado
    dejarTerminar(c, t);
    e.registrarOrientacion(malOrientada(80));
    e.registrarOrientacion(invalida());
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::NINGUNO);
    VERIFICAR(!e.orientado());  // la lectura inválida (que dice "de frente") no cuenta
    e.registrarOrientacion(malOrientada(80));
    VERIFICAR_IGUAL(e.orquestar(3010), Patron::GIRAR_DERECHA);
}

PRUEBA(evaluador_lectura_invalida_no_cambia_el_estado_en_ningun_sentido) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    orientarDeFrente(e);
    VERIFICAR(e.orientado());
    e.registrarOrientacion({false, 120.0f, 120.0f});  // inválida y "mal orientada"
    VERIFICAR(e.orientado());
    e.registrarOrientacion(malOrientada(60));
    VERIFICAR(!e.orientado());
    e.registrarOrientacion(invalida());               // inválida y "de frente"
    VERIFICAR(!e.orientado());
}

PRUEBA(evaluador_lectura_invalida_corta_solo_el_pulso_de_giro) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 1));
    e.orquestar(t);
    dejarTerminar(c, t);
    e.registrarOrientacion(malOrientada(80));
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::GIRAR_DERECHA);
    VERIFICAR(c.sonando());
    e.registrarOrientacion(invalida());
    VERIFICAR(!c.sonando());  // cortado: podía señalar el lado equivocado

    // Un aviso de luz no se corta por una lectura inválida
    orientarDeFrente(e);
    VERIFICAR_IGUAL(e.orquestar(6000), Patron::CRUCE_PERMITIDO);
    e.registrarOrientacion(invalida());
    VERIFICAR(c.sonando());
    VERIFICAR_IGUAL(c.patronActual(), Patron::CRUCE_PERMITIDO);
}

PRUEBA(evaluador_cambio_de_luz_durante_un_aviso_espera_su_turno) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 1));
    orientarDeFrente(e);
    e.orquestar(t);
    dejarTerminar(c, t);
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::CRUCE_PERMITIDO);  // dura 4.5 s
    e.registrarPaquete(paquete(1, 90, 0));                        // pasa a rojo a los 4 s
    e.registrarPaquete(paquete(1, 90, 0));                        // y se confirma
    const uint32_t libre = 3000 + 4500 + SILENCIO_TRAS_PATRON_MS;
    for (t = 4000; t < libre; t += 10) {
        c.actualizar(t);
        VERIFICAR_IGUAL(e.orquestar(t), Patron::NINGUNO);
    }
    VERIFICAR_IGUAL(e.orquestar(libre), Patron::CRUCE_PROHIBIDO);
}

PRUEBA(evaluador_paquete_de_otro_id_no_toca_los_avisos) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 1));
    orientarDeFrente(e);
    e.orquestar(t);
    dejarTerminar(c, t);
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::CRUCE_PERMITIDO);
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(2, 270, 0)), CambioPaquete::IGNORADO);
    VERIFICAR(e.cruceNotificado());
    VERIFICAR_IGUAL(e.orquestar(20000), Patron::NINGUNO);
}

PRUEBA(evaluador_giro_cada_tres_segundos_tras_el_desborde) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t0 = 0xFFFFF000u;
    e.registrarPaquete(paquete(1, 90, 0));
    VERIFICAR_IGUAL(e.orquestar(t0), Patron::SEMAFORO_DETECTADO);
    e.registrarOrientacion(malOrientada(45));
    VERIFICAR_IGUAL(e.orquestar((uint32_t)(t0 + 3000)), Patron::GIRAR_DERECHA);
    VERIFICAR_IGUAL(e.orquestar((uint32_t)(t0 + 5999)), Patron::NINGUNO);
    VERIFICAR_IGUAL(e.orquestar((uint32_t)(t0 + 6000)), Patron::GIRAR_DERECHA);
}

PRUEBA(evaluador_lado_del_giro_en_los_empates) {
    // giro > 0 → derecha; giro <= 0 → izquierda (con datos reales |giro| es la
    // desviación, así que 0 no ocurre estando mal orientada; 180 sí: de espaldas)
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 0));
    e.orquestar(0);
    e.registrarOrientacion({true, 180.0f, 180.0f});
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::GIRAR_DERECHA);
    e.registrarOrientacion({true, 180.0f, 0.0f});
    VERIFICAR_IGUAL(e.orquestar(6000), Patron::GIRAR_IZQUIERDA);
}

PRUEBA(evaluador_reiniciar_durante_un_aviso_largo) {
    // Al pasar a standby a mitad de un verde (4.5 s), la FSM lo corta; si el
    // semáforo vuelve enseguida, "detectado" espera duración completa + descanso
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 1));
    orientarDeFrente(e);
    c.iniciar(Patron::CRUCE_PERMITIDO, 0);
    c.cancelar();
    e.reiniciar();
    e.registrarPaquete(paquete(1, 90, 1));
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::NINGUNO);
    VERIFICAR_IGUAL(e.orquestar(4500 + SILENCIO_TRAS_PATRON_MS - 1), Patron::NINGUNO);
    VERIFICAR_IGUAL(e.orquestar(4500 + SILENCIO_TRAS_PATRON_MS), Patron::SEMAFORO_DETECTADO);
}

PRUEBA(evaluador_reiniciar_respeta_la_separacion_minima) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 0));
    VERIFICAR_IGUAL(e.orquestar(10000), Patron::SEMAFORO_DETECTADO);
    c.cancelar();  // la FSM corta todo al entrar a standby
    e.reiniciar();
    e.registrarPaquete(paquete(1, 90, 0));  // el semáforo vuelve enseguida
    VERIFICAR_IGUAL(e.orquestar(11000), Patron::NINGUNO);
    VERIFICAR_IGUAL(e.orquestar(12999), Patron::NINGUNO);
    VERIFICAR_IGUAL(e.orquestar(13000), Patron::SEMAFORO_DETECTADO);
}

PRUEBA(evaluador_antes_del_aviso_el_cono_es_de_cinco_grados) {
    // De frente confirmado pero la luz todavía espera turno (motores ocupados):
    // si la persona sigue girando y sale del cono de 5°, ya no suena la luz al
    // quedar libre (estaría pasada); se vuelve a indicar el giro
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 1));
    VERIFICAR_IGUAL(e.orquestar(0), Patron::SEMAFORO_DETECTADO);
    orientarDeFrente(e);
    VERIFICAR(e.orientado());
    e.registrarOrientacion({true, 7.0f, -7.0f});  // se pasó, dentro de la histéresis
    VERIFICAR(!e.orientado());
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::GIRAR_IZQUIERDA);
}

PRUEBA(evaluador_no_repite_el_giro_si_ya_se_acerca) {
    // Pulso a la derecha; la persona gira hacia el cruce: no hay segundo pulso
    // ("sigue girando") que ocupe los motores justo cuando va a llegar
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 1));
    e.orquestar(t);
    dejarTerminar(c, t);
    e.registrarOrientacion(malOrientada(40));
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::GIRAR_DERECHA);
    t = 3000;
    dejarTerminar(c, t);
    float d = 40.0f;
    for (int i = 0; i < 15; ++i) {          // 3 s girando a ~10°/s (lecturas a 5 Hz)
        d -= 2.0f;
        e.registrarOrientacion({true, d, d});
        VERIFICAR_IGUAL(e.orquestar(3000 + 200 * (uint32_t)(i + 1)), Patron::NINGUNO);
    }
    // Se queda quieta antes de llegar: el recordatorio vuelve
    for (int i = 0; i < LECTURAS_ACERCAMIENTO; ++i) e.registrarOrientacion({true, d, d});
    VERIFICAR_IGUAL(e.orquestar(7000), Patron::GIRAR_DERECHA);
}

PRUEBA(evaluador_cruzar_el_objetivo_no_cuenta_como_acercarse) {
    // Al pasar de un lado al otro la desviación baja y vuelve a subir: tiene
    // que llegar el pulso hacia el otro lado
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 1));
    e.orquestar(0);
    e.registrarOrientacion({true, 12.0f, 12.0f});
    e.registrarOrientacion({true, 8.0f, -8.0f});
    e.registrarOrientacion({true, 7.0f, -7.0f});
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::GIRAR_IZQUIERDA);
}
