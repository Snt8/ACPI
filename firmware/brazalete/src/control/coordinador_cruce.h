#ifndef COORDINADOR_CRUCE_H
#define COORDINADOR_CRUCE_H

#include <stdint.h>
#include "evaluador_semaforo.h"
#include "../vibracion/controlador_patrones.h"
#include "../protocolo/protocolo_esp.h"

// ── Coordinador del estado SENSING_CROSSING ──────────────────────────────────
// Una vuelta del loop mientras hay un semáforo a la vista, sin hardware:
//   1. pasa el paquete nuevo (si llegó) al evaluador;
//   2. cierra el patrón que ya terminó;
//   3. a 2 Hz, y sólo si no vibra ningún motor (sus imanes alteran el
//      magnetómetro), pide una lectura de orientación y se la pasa al evaluador;
//   4. deja que el evaluador inicie el patrón que corresponda;
//   5. devuelve qué motor debe estar encendido.
//
// La FSM lo usa con la brújula y los motores reales, y las pruebas de escenario
// de test/ con lecturas simuladas: así ambos ejecutan exactamente la misma lógica.

// Lee la orientación de la persona respecto al semáforo. 'contexto' es el puntero
// que se pasó a paso() (la FSM no lo usa; los tests, sí).
typedef Orientacion (*LectorOrientacion)(const estado_semaforo& semaforo, void* contexto);

// Lo que pasó en una vuelta, para registrarlo en el monitor serie
struct VueltaCruce {
    bool          hubo_paquete;  // llegó un paquete en esta vuelta
    CambioPaquete cambio;        // qué diferencia tenía con el anterior (si hubo_paquete)
    bool          muestreo;      // se leyó la orientación
    Patron        iniciado;      // patrón que empezó en esta vuelta (o NINGUNO)
    SalidaMotores salida;        // estado que deben tener los motores
};

class CoordinadorCruce {
public:
    CoordinadorCruce();

    // Al pasar a SENSING_CROSSING: reinicia los relojes de paquetes y muestreo
    void entrar(uint32_t ahora);

    // Al dejar SENSING_CROSSING: corta el patrón en curso y olvida el semáforo.
    // El controlador conserva la hora del último patrón, así la separación
    // mínima se respeta aunque el semáforo vuelva a aparecer enseguida.
    void salir();

    VueltaCruce paso(uint32_t ahora, const estado_semaforo* paquete,
                     LectorOrientacion leer, void* contexto);

    // true si pasaron TIEMPO_SIN_PAQUETES_STANDBY_MS sin paquetes del semáforo
    // que se sigue (los de otro id no cuentan)
    bool sinPaquetes(uint32_t ahora) const;

    const EvaluadorSemaforo&   evaluador() const { return evaluador_; }
    const ControladorPatrones& patrones() const { return patrones_; }

private:
    ControladorPatrones patrones_;   // debe declararse antes que el evaluador
    EvaluadorSemaforo   evaluador_;
    uint32_t            ultimo_paquete_;
    uint32_t            ultimo_muestreo_;
};

#endif // COORDINADOR_CRUCE_H
