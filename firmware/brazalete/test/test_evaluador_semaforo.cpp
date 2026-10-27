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

Orientacion deFrente()              { return {true, 5.0f, 5.0f}; }
Orientacion malOrientada(float giro) { return {true, 90.0f, giro}; }
Orientacion conDesviacion(float d)   { return {true, d, d}; }
// Una lectura inválida "dice" estar de frente: si el evaluador la usara por
// error, cambiaría de estado y las pruebas lo notarían
Orientacion invalida()               { return {false, 0.0f, 0.0f}; }

// Avanza el controlador hasta que termine el patrón en curso
void dejarTerminar(ControladorPatrones& c, uint32_t& t) {
    while (c.sonando()) c.actualizar(++t);
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
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarPaquete(paquete(1, 90, 0));
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 1)), CambioPaquete::CAMBIO_A_PERMITIDO);
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 1)), CambioPaquete::SIN_CAMBIOS);
    VERIFICAR_IGUAL(e.registrarPaquete(paquete(1, 90, 0)), CambioPaquete::CAMBIO_A_PROHIBIDO);
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
    e.registrarOrientacion(deFrente());
    VERIFICAR_IGUAL(e.orquestar(t), Patron::SEMAFORO_DETECTADO);
    dejarTerminar(c, t);
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::CRUCE_PERMITIDO);
    VERIFICAR(e.cruceNotificado());
    t = 3000;
    // Persona quieta y misma luz durante un minuto: silencio absoluto
    for (t += 10; t < 63000; t += 10) {
        e.registrarPaquete(paquete(1, 90, 1));
        e.registrarOrientacion(deFrente());
        c.actualizar(t);
        VERIFICAR_IGUAL(e.orquestar(t), Patron::NINGUNO);
    }
}

PRUEBA(evaluador_cambio_de_luz_de_frente_vibra_una_vez) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 0));
    e.registrarOrientacion(deFrente());
    e.orquestar(t);  // detectado
    dejarTerminar(c, t);
    t = 3000;
    VERIFICAR_IGUAL(e.orquestar(t), Patron::CRUCE_PROHIBIDO);
    dejarTerminar(c, t);

    t = 20000;
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
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::GIRAR_DERECHA);  // primero orientarse
    t = 3000;
    dejarTerminar(c, t);
    e.registrarOrientacion(deFrente());
    VERIFICAR_IGUAL(e.orquestar(6000), Patron::CRUCE_PERMITIDO);  // la luz actual
}

PRUEBA(evaluador_histeresis_de_orientacion) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS - 1));
    VERIFICAR(e.orientado());
    // Entre el umbral y umbral + histéresis sigue "de frente"
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS + 5));
    VERIFICAR(e.orientado());
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS + HISTERESIS_ORIENTACION_GRADOS));
    VERIFICAR(e.orientado());
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS + HISTERESIS_ORIENTACION_GRADOS + 0.5f));
    VERIFICAR(!e.orientado());
    // Para volver a "de frente" hay que bajar del umbral, no de umbral + histéresis
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS + 5));
    VERIFICAR(!e.orientado());
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS));
    VERIFICAR(!e.orientado());  // el umbral es estricto (<)
    e.registrarOrientacion(conDesviacion(UMBRAL_CRUCE_GRADOS - 0.5f));
    VERIFICAR(e.orientado());
}

PRUEBA(evaluador_volver_a_quedar_de_frente_repite_la_luz) {
    ControladorPatrones c;
    EvaluadorSemaforo e(c);
    uint32_t t = 0;
    e.registrarPaquete(paquete(1, 90, 0));
    e.registrarOrientacion(deFrente());
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
    e.registrarOrientacion(deFrente());
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
    e.registrarOrientacion(deFrente());
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
    e.registrarOrientacion(deFrente());
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
    e.registrarOrientacion(deFrente());
    e.orquestar(t);
    dejarTerminar(c, t);
    VERIFICAR_IGUAL(e.orquestar(3000), Patron::CRUCE_PERMITIDO);  // dura 4.5 s
    e.registrarPaquete(paquete(1, 90, 0));                        // pasa a rojo a los 4 s
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
    e.registrarOrientacion(deFrente());
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
    e.registrarOrientacion(deFrente());
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
