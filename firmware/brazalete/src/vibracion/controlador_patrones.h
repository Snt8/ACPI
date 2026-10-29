#ifndef CONTROLADOR_PATRONES_H
#define CONTROLADOR_PATRONES_H

#include <stdint.h>

// ── Controlador de patrones de vibración ─────────────────────────────────────
// Reproduce UNA vez el patrón que se le pide y dice en cada instante qué motor
// debe estar encendido. No toca el hardware ni llama a millis(): recibe el
// tiempo y devuelve el estado de los motores, así se puede probar en la PC
// (test/) y la FSM es la única que escribe en los pines.
//
// Garantías (protegen las soldaduras de los motores):
//   - Sin una llamada a iniciar() los motores nunca se encienden.
//   - Un patrón termina solo; nunca se repite por su cuenta.
//   - Dos patrones nunca se solapan, y entre el inicio de uno y el del
//     siguiente pasan al menos SEPARACION_MINIMA_EVENTOS_MS (constantes.h).
//   - Tras terminar un patrón, los motores descansan al menos
//     SILENCIO_TRAS_PATRON_MS antes del siguiente.
//
// Los tiempos son uint32_t en ms, como millis() en el ESP32; las restas sin
// signo hacen que el desborde de millis() (cada ~49 días) no afecte.

enum class Patron : uint8_t {
    NINGUNO,
    SEMAFORO_DETECTADO,  // 3 pulsos cortos, ambos motores
    GIRAR_DERECHA,       // 1 pulso, motor derecho
    GIRAR_IZQUIERDA,     // 1 pulso, motor izquierdo
    CRUCE_PERMITIDO,     // semáforo peatonal en verde: 2 pulsos de 2 s, ambos
    CRUCE_PROHIBIDO      // semáforo peatonal en rojo: 1 pulso de 2 s, ambos
};

// Qué motor debe estar encendido en este instante
struct SalidaMotores {
    bool derecho;
    bool izquierdo;

    bool operator==(const SalidaMotores& otra) const {
        return derecho == otra.derecho && izquierdo == otra.izquierdo;
    }
    bool operator!=(const SalidaMotores& otra) const { return !(*this == otra); }
};

class ControladorPatrones {
public:
    ControladorPatrones();

    // true si se puede iniciar un patrón ahora: no hay ninguno sonando, ya pasó
    // la separación mínima desde el inicio del anterior y el silencio tras su fin
    bool libre(uint32_t ahora) const;

    // Inicia el patrón. Devuelve false (y no hace nada) si no está libre o si
    // el patrón es NINGUNO.
    bool iniciar(Patron patron, uint32_t ahora);

    // Corta el patrón en curso y apaga los motores. La separación y el silencio
    // se cuentan como si hubiera terminado entero: cancelar no adelanta nada.
    void cancelar();

    // Estado de los motores en este instante. Cuando el patrón llega a su fin,
    // lo da por terminado y devuelve ambos motores apagados.
    SalidaMotores actualizar(uint32_t ahora);

    bool sonando() const { return actual_ != Patron::NINGUNO; }

    // true si ningún motor vibra y el último patrón terminó hace al menos
    // 'ms' (un patrón cortado cuenta como si hubiera sonado entero)
    bool quietosDesde(uint32_t ahora, uint32_t ms) const;
    Patron patronActual() const { return actual_; }

    // Duración total de un patrón en ms (pulsos + pausas entre ellos)
    static uint32_t duracion(Patron patron);

private:
    Patron   actual_;
    uint32_t inicio_;           // millis() del último iniciar() aceptado
    uint32_t duracion_ultimo_;  // duración completa del último patrón iniciado
    bool     hubo_inicio_;  // false hasta el primer patrón (no hay separación que respetar)
};

#endif // CONTROLADOR_PATRONES_H
