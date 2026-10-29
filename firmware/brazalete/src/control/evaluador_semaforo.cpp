#include "evaluador_semaforo.h"
#include "../config.h"
#include "../constantes.h"
#include <math.h>

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
    lecturas_de_frente_   = 0;
    n_historial_          = 0;
    giro_                 = 0.0f;
    hubo_giro_            = false;
    ultimo_giro_          = 0;
    luz_por_confirmar_    = 0;
    confirmaciones_       = 0;
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

    if (actual.permitidoCruce == anterior_.permitidoCruce) {
        confirmaciones_ = 0;  // un cambio sin confirmar que no se repitió se olvida
        anterior_ = actual;   // grados y tiempo se actualizan aunque no haya evento
        return CambioPaquete::SIN_CAMBIOS;
    }

    // La luz es distinta a la vigente: hay que verla CONFIRMACIONES_CAMBIO_LUZ
    // veces seguidas antes de creerla
    if (confirmaciones_ == 0 || luz_por_confirmar_ != actual.permitidoCruce) {
        luz_por_confirmar_ = actual.permitidoCruce;
        confirmaciones_    = 1;
    } else {
        confirmaciones_++;
    }
    if (confirmaciones_ < CONFIRMACIONES_CAMBIO_LUZ) {
        anterior_.grados = actual.grados;  // la luz y su tiempo siguen siendo los vigentes
        return CambioPaquete::CAMBIO_POR_CONFIRMAR;
    }

    confirmaciones_   = 0;
    anterior_         = actual;
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
        // Una lectura inválida no cambia el estado de orientación, pero corta
        // la racha: "de frente" se confirma con lecturas válidas SEGUIDAS
        lecturas_de_frente_ = 0;
        n_historial_ = 0;
        return;
    }

    orientacion_conocida_ = true;
    giro_ = orientacion.giro;

    // Historial con signo: + a la derecha del cruce, − a la izquierda
    float con_signo = (orientacion.giro > 0.0f) ? orientacion.desviacion : -orientacion.desviacion;
    for (int i = LECTURAS_ACERCAMIENTO - 1; i > 0; --i) historial_[i] = historial_[i - 1];
    historial_[0] = con_signo;
    if (n_historial_ < LECTURAS_ACERCAMIENTO) n_historial_++;

    // Histéresis: entra a "de frente" bajo UMBRAL (confirmado con
    // CONFIRMACIONES_ORIENTACION lecturas seguidas) y sale con UNA lectura
    // sobre UMBRAL + HISTERESIS, para volver a indicar el giro cuanto antes.
    // Mientras la luz todavía no se avisó, sale apenas deja el cono: el aviso
    // es la señal de "para" y no puede llegar con la persona ya pasada.
    if (orientado_) {
        bool salio = cruce_notificado_
                   ? orientacion.desviacion > UMBRAL_CRUCE_GRADOS + HISTERESIS_ORIENTACION_GRADOS
                   : orientacion.desviacion >= UMBRAL_CRUCE_GRADOS;
        if (salio) {
            orientado_ = false;
            lecturas_de_frente_ = 0;
            cruce_notificado_ = false;  // al volver a quedar de frente se avisa la luz otra vez
        }
    } else if (orientacion.desviacion < UMBRAL_CRUCE_GRADOS) {
        if (++lecturas_de_frente_ >= CONFIRMACIONES_ORIENTACION) {
            orientado_ = true;
        }
    } else {
        lecturas_de_frente_ = 0;
    }
}

bool EvaluadorSemaforo::acercandose() const {
    if (n_historial_ < LECTURAS_ACERCAMIENTO) return false;
    const float actual = historial_[0];
    const float antigua = historial_[LECTURAS_ACERCAMIENTO - 1];
    for (int i = 0; i < LECTURAS_ACERCAMIENTO; ++i) {
        if ((historial_[i] > 0.0f) != (actual > 0.0f)) return false;  // cruzó el objetivo
    }
    return fabsf(antigua) - fabsf(actual) >= UMBRAL_ACERCAMIENTO_GRADOS;
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

    // 2. Mal orientada: un pulso hacia el lado del giro cada INTERVALO_GIRO_MS.
    //    Si la última lectura ya cayó dentro del umbral y sólo falta
    //    confirmarla, se espera: un pulso ahora la empujaría fuera del cono.
    //    Tampoco se repite si ya está girando hacia el cruce: así los motores
    //    quedan libres para el aviso de la luz en cuanto llegue.
    if (!orientado_) {
        if (lecturas_de_frente_ > 0) return Patron::NINGUNO;
        if (acercandose()) return Patron::NINGUNO;
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
