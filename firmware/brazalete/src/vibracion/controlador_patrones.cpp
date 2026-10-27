#include "controlador_patrones.h"
#include "../constantes.h"

// ── Tabla de patrones ────────────────────────────────────────────────────────
// Todos los patrones son "N pulsos iguales separados por una pausa", así que
// cada uno se describe con estos cuatro datos (valores en constantes.h).
namespace {

struct Forma {
    uint8_t  pulsos;
    uint32_t pulso_ms;
    uint32_t pausa_ms;
    bool     derecho;
    bool     izquierdo;
};

Forma formaDe(Patron patron) {
    switch (patron) {
        case Patron::SEMAFORO_DETECTADO:
            return {DETECTADO_PULSOS, DETECTADO_PULSO_MS, DETECTADO_PAUSA_MS, true, true};
        case Patron::GIRAR_DERECHA:
            return {1, GIRO_PULSO_MS, 0, true, false};
        case Patron::GIRAR_IZQUIERDA:
            return {1, GIRO_PULSO_MS, 0, false, true};
        case Patron::CRUCE_PERMITIDO:
            return {PERMITIDO_PULSOS, PERMITIDO_PULSO_MS, PERMITIDO_PAUSA_MS, true, true};
        case Patron::CRUCE_PROHIBIDO:
            return {PROHIBIDO_PULSOS, PROHIBIDO_PULSO_MS, 0, true, true};
        case Patron::NINGUNO:
        default:
            return {0, 0, 0, false, false};
    }
}

const SalidaMotores APAGADOS = {false, false};

} // namespace

// ── Implementación ───────────────────────────────────────────────────────────
ControladorPatrones::ControladorPatrones()
    : actual_(Patron::NINGUNO), inicio_(0), hubo_inicio_(false) {}

uint32_t ControladorPatrones::duracion(Patron patron) {
    Forma f = formaDe(patron);
    if (f.pulsos == 0) return 0;
    return f.pulsos * f.pulso_ms + (f.pulsos - 1) * f.pausa_ms;
}

bool ControladorPatrones::libre(uint32_t ahora) const {
    if (!hubo_inicio_) return true;
    uint32_t transcurrido = ahora - inicio_;
    // Un patrón que ya llegó a su fin cuenta como terminado aunque actualizar()
    // todavía no lo haya registrado
    if (sonando() && transcurrido < duracion(actual_)) return false;
    return transcurrido >= SEPARACION_MINIMA_EVENTOS_MS;
}

bool ControladorPatrones::iniciar(Patron patron, uint32_t ahora) {
    if (patron == Patron::NINGUNO || !libre(ahora)) return false;
    actual_      = patron;
    inicio_      = ahora;
    hubo_inicio_ = true;
    return true;
}

void ControladorPatrones::cancelar() {
    actual_ = Patron::NINGUNO;  // inicio_ no se toca: la separación sigue corriendo
}

SalidaMotores ControladorPatrones::actualizar(uint32_t ahora) {
    if (!sonando()) return APAGADOS;

    uint32_t transcurrido = ahora - inicio_;
    if (transcurrido >= duracion(actual_)) {
        actual_ = Patron::NINGUNO;  // terminó: no se repite
        return APAGADOS;
    }

    // Dentro de un ciclo pulso+pausa: encendido durante el pulso, apagado en la pausa
    Forma f = formaDe(actual_);
    uint32_t ciclo = f.pulso_ms + f.pausa_ms;
    bool en_pulso  = (transcurrido % ciclo) < f.pulso_ms;
    if (!en_pulso) return APAGADOS;
    return {f.derecho, f.izquierdo};
}
