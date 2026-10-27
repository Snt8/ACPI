#include "ciclo_escucha.h"
#include "../constantes.h"

// true si el instante 'a' ya llegó o pasó 'b', aunque millis() se haya desbordado
static bool yaLlego(uint32_t a, uint32_t b) {
    return (int32_t)(a - b) >= 0;
}

CicloEscucha::CicloEscucha() : escuchando_(false), inicio_ventana_(0), proxima_(0) {}

void CicloEscucha::iniciar(uint32_t ahora, bool escucharYa) {
    escuchando_     = false;
    inicio_ventana_ = 0;
    proxima_        = escucharYa ? ahora : ahora + INTERVALO_ESCUCHA_STANDBY_MS;
}

CicloEscucha::Accion CicloEscucha::actualizar(uint32_t ahora) {
    if (escuchando_) {
        if ((uint32_t)(ahora - inicio_ventana_) >= VENTANA_ESCUCHA_STANDBY_MS) {
            escuchando_ = false;
            // Si la ventana se cerró tarde y la siguiente ya debería haber
            // empezado, se deja el apagado completo: nunca dos ventanas seguidas
            if (yaLlego(ahora, proxima_)) {
                proxima_ = ahora + (INTERVALO_ESCUCHA_STANDBY_MS - VENTANA_ESCUCHA_STANDBY_MS);
            }
            return Accion::APAGAR_RADIO;
        }
        return Accion::NADA;
    }

    if (yaLlego(ahora, proxima_)) {
        // Si la CPU despertó tarde, la ventana empieza ahora y la siguiente se
        // cuenta desde aquí
        escuchando_     = true;
        inicio_ventana_ = ahora;
        proxima_        = ahora + INTERVALO_ESCUCHA_STANDBY_MS;
        return Accion::ENCENDER_RADIO;
    }
    return Accion::NADA;
}

uint32_t CicloEscucha::msHastaProximaEscucha(uint32_t ahora) const {
    if (escuchando_ || yaLlego(ahora, proxima_)) return 0;
    return proxima_ - ahora;
}
