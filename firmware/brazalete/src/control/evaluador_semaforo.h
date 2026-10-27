#ifndef EVALUADOR_SEMAFORO_H
#define EVALUADOR_SEMAFORO_H

#include <stdint.h>
#include "../protocolo/protocolo_esp.h"
#include "../vibracion/controlador_patrones.h"

// ── Evaluador de paquetes del semáforo ───────────────────────────────────────
// Compara cada paquete con el anterior para detectar EVENTOS (semáforo
// encontrado, cambio de luz) y, junto con la orientación de la persona,
// decide qué patrón pide al ControladorPatrones. Es el orquestador de la
// vibración: la FSM sólo le pasa paquetes y lecturas, y aplica a los motores
// lo que devuelve el controlador.
//
// Como el controlador, no usa Arduino ni millis(): se prueba en la PC (test/).
//
// Eventos que hacen vibrar (cada uno UNA vez, ver patrones.md):
//   - Primer paquete de un semáforo          -> SEMAFORO_DETECTADO
//   - Persona mal orientada                  -> GIRAR_* cada INTERVALO_GIRO_MS
//   - Queda de frente al cruce               -> CRUCE_PERMITIDO / CRUCE_PROHIBIDO
//   - Cambia la luz estando de frente        -> CRUCE_PERMITIDO / CRUCE_PROHIBIDO
// Cualquier otro paquete (mismo estado, sólo cambian grados o tiempo) no
// produce vibración.

// Resultado de comparar un paquete con el anterior
enum class CambioPaquete : uint8_t {
    IGNORADO,         // de otro semáforo (otro id) mientras se sigue uno
    SIN_CAMBIOS,      // misma luz que el paquete anterior
    SEMAFORO_NUEVO,   // primer paquete desde reiniciar()
    CAMBIO_A_PERMITIDO,
    CAMBIO_A_PROHIBIDO
};

// Lectura de orientación de la persona respecto al cruce
struct Orientacion {
    bool  valida;      // false si la brújula rechazó la lectura (safety check)
    float desviacion;  // |Δθ − 180°| en grados [0, 180]; 0 = de frente al cruce
    float giro;        // giro más corto hacia el cruce; > 0 derecha, < 0 izquierda
};

class EvaluadorSemaforo {
public:
    explicit EvaluadorSemaforo(ControladorPatrones& patrones);

    // Olvida el semáforo y la orientación (al entrar en standby). No toca el
    // historial del controlador: la separación mínima entre patrones se sigue
    // respetando aunque el brazalete vuelva de standby enseguida.
    void reiniciar();

    // Compara el paquete con el anterior y registra el evento que haya.
    // Se queda con el primer id que escucha: paquetes de otro id se ignoran
    // hasta reiniciar(), para que dos semáforos cercanos no se alternen y
    // hagan vibrar "semáforo detectado" una y otra vez.
    CambioPaquete registrarPaquete(const estado_semaforo& paquete);

    // Actualiza si la persona está de frente al cruce (con histéresis).
    // Si la lectura es inválida y está sonando un pulso de orientación, lo
    // corta: podría estar indicando el lado equivocado.
    void registrarOrientacion(const Orientacion& orientacion);

    // Si el controlador está libre y hay un evento pendiente, inicia su patrón.
    // Devuelve el patrón iniciado, o NINGUNO. Llamar en cada vuelta del loop.
    Patron orquestar(uint32_t ahora);

    bool haySemaforo() const { return hay_semaforo_; }
    bool orientado() const { return orientado_; }
    bool cruceNotificado() const { return cruce_notificado_; }
    const estado_semaforo& ultimoPaquete() const { return anterior_; }

private:
    // Decide el patrón que corresponde ahora y marca su evento como atendido.
    // Sólo se llama cuando el controlador está libre, así el patrón se inicia seguro.
    Patron decidir(uint32_t ahora);

    ControladorPatrones& patrones_;

    // Paquetes
    bool            hay_semaforo_;
    estado_semaforo anterior_;
    bool            detectado_pendiente_;  // falta avisar "semáforo detectado"
    bool            cruce_notificado_;     // ya se avisó la luz actual estando de frente

    // Orientación
    bool  orientacion_conocida_;  // hubo al menos una lectura válida
    bool  ultima_valida_;
    bool  orientado_;
    float giro_;
    bool     hubo_giro_;
    uint32_t ultimo_giro_;        // inicio del último pulso de orientación
};

#endif // EVALUADOR_SEMAFORO_H
