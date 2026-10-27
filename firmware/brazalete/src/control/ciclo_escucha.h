#ifndef CICLO_ESCUCHA_H
#define CICLO_ESCUCHA_H

#include <stdint.h>

// ── Ciclo de escucha del modo standby ────────────────────────────────────────
// Decide cuándo se enciende y se apaga la radio en standby: una ventana de
// VENTANA_ESCUCHA_STANDBY_MS cada INTERVALO_ESCUCHA_STANDBY_MS (inicio a
// inicio). El resto del tiempo la radio está apagada.
//
// No toca el hardware ni llama a millis(): ModoStandby ejecuta las acciones
// que devuelve, y las pruebas de test/ verifican los tiempos en la PC.
class CicloEscucha {
public:
    enum class Accion : uint8_t { NADA, ENCENDER_RADIO, APAGAR_RADIO };

    CicloEscucha();

    // Empieza con la radio apagada. Si escucharYa, la primera ventana se abre
    // en el próximo actualizar(); si no, dentro de un intervalo completo.
    void iniciar(uint32_t ahora, bool escucharYa);

    // Qué hacer con la radio ahora. Devuelve ENCENDER_RADIO al abrir una
    // ventana, APAGAR_RADIO al cerrarla y NADA el resto del tiempo.
    Accion actualizar(uint32_t ahora);

    bool escuchando() const { return escuchando_; }

    // ms que faltan para la próxima ventana (0 si está escuchando o ya toca).
    // ModoStandby duerme este tiempo.
    uint32_t msHastaProximaEscucha(uint32_t ahora) const;

private:
    bool     escuchando_;
    uint32_t inicio_ventana_;  // inicio de la ventana abierta
    uint32_t proxima_;         // inicio programado de la próxima ventana
};

#endif // CICLO_ESCUCHA_H
