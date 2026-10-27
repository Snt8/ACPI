#include "evaluador_semaforo.h"
#include "../config.h"
#include "../constantes.h"

EvaluadorSemaforo::EvaluadorSemaforo(ControladorPatrones& patrones)
    : patrones_(patrones) {
    reiniciar();
}

void EvaluadorSemaforo::reiniciar() {
    hay_semaforo_         = false;
    anterior_             = estado_semaforo{};
    detectado_pendiente_  = false;
    cruce_notificado_     = false;
    orientacion_conocida_ = false;
    ultima_valida_        = false;
    orientado_            = false;
    giro_                 = 0.0f;
    hubo_giro_            = false;
    ultimo_giro_          = 0;
}

// ── Paquetes ─────────────────────────────────────────────────────────────────
CambioPaquete EvaluadorSemaforo::registrarPaquete(const estado_semaforo& paquete) {
    // Cualquier valor distinto de 0 cuenta como "permitido", así un 2 no se
    // confunde con un cambio de luz respecto a un 1
    estado_semaforo actual = paquete;
    actual.permitidoCruce = paquete.permitidoCruce ? 1 : 0;

    if (!hay_semaforo_) {
        hay_semaforo_        = true;
        anterior_            = actual;
        detectado_pendiente_ = true;
        cruce_notificado_    = false;
        return CambioPaquete::SEMAFORO_NUEVO;
    }

    if (actual.id != anterior_.id) {
        return CambioPaquete::IGNORADO;
    }

    bool cambio_luz = (actual.permitidoCruce != anterior_.permitidoCruce);
    anterior_ = actual;  // grados y tiempo se actualizan aunque no haya evento
    if (!cambio_luz) {
        return CambioPaquete::SIN_CAMBIOS;
    }

    cruce_notificado_ = false;  // la luz nueva todavía no se avisó
    return actual.permitidoCruce ? CambioPaquete::CAMBIO_A_PERMITIDO
                                 : CambioPaquete::CAMBIO_A_PROHIBIDO;
}

// ── Orientación ──────────────────────────────────────────────────────────────
void EvaluadorSemaforo::registrarOrientacion(const Orientacion& orientacion) {
    ultima_valida_ = orientacion.valida;

    if (!orientacion.valida) {
        Patron p = patrones_.patronActual();
        if (p == Patron::GIRAR_DERECHA || p == Patron::GIRAR_IZQUIERDA) {
            patrones_.cancelar();
        }
        return;  // una lectura inválida no cambia el estado de orientación
    }

    orientacion_conocida_ = true;
    giro_ = orientacion.giro;

    // Histéresis: entra a "de frente" bajo UMBRAL y sale sólo sobre UMBRAL + HISTERESIS
    if (orientado_) {
        if (orientacion.desviacion > UMBRAL_CRUCE_GRADOS + HISTERESIS_ORIENTACION_GRADOS) {
            orientado_ = false;
            cruce_notificado_ = false;  // al volver a quedar de frente se avisa la luz otra vez
        }
    } else if (orientacion.desviacion < UMBRAL_CRUCE_GRADOS) {
        orientado_ = true;
    }
}

// ── Orquestación ─────────────────────────────────────────────────────────────
Patron EvaluadorSemaforo::orquestar(uint32_t ahora) {
    if (!patrones_.libre(ahora)) return Patron::NINGUNO;

    Patron patron = decidir(ahora);
    if (patron != Patron::NINGUNO) {
        patrones_.iniciar(patron, ahora);  // siempre acepta: se comprobó libre()
    }
    return patron;
}

Patron EvaluadorSemaforo::decidir(uint32_t ahora) {
    if (!hay_semaforo_) return Patron::NINGUNO;

    // 1. El aviso de semáforo detectado no depende de la brújula
    if (detectado_pendiente_) {
        detectado_pendiente_ = false;
        return Patron::SEMAFORO_DETECTADO;
    }

    // Sin una orientación fiable no se indica nada más
    if (!orientacion_conocida_ || !ultima_valida_) return Patron::NINGUNO;

    // 2. Mal orientada: un pulso hacia el lado del giro cada INTERVALO_GIRO_MS
    if (!orientado_) {
        if (hubo_giro_ && (uint32_t)(ahora - ultimo_giro_) < INTERVALO_GIRO_MS) {
            return Patron::NINGUNO;
        }
        hubo_giro_   = true;
        ultimo_giro_ = ahora;
        return (giro_ > 0.0f) ? Patron::GIRAR_DERECHA : Patron::GIRAR_IZQUIERDA;
    }

    // 3. De frente: avisar la luz actual una sola vez
    if (!cruce_notificado_) {
        cruce_notificado_ = true;
        return anterior_.permitidoCruce ? Patron::CRUCE_PERMITIDO : Patron::CRUCE_PROHIBIDO;
    }

    return Patron::NINGUNO;  // sin eventos: los motores no se tocan
}
