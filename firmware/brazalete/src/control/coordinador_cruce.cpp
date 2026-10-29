#include "coordinador_cruce.h"
#include "../constantes.h"

CoordinadorCruce::CoordinadorCruce()
    : evaluador_(patrones_), ultimo_paquete_(0), ultimo_muestreo_(0) {}

void CoordinadorCruce::entrar(uint32_t ahora) {
    ultimo_paquete_  = ahora;
    ultimo_muestreo_ = ahora;
}

void CoordinadorCruce::salir() {
    patrones_.cancelar();
    evaluador_.reiniciar();
}

VueltaCruce CoordinadorCruce::paso(uint32_t ahora, const estado_semaforo* paquete,
                                   LectorOrientacion leer, void* contexto) {
    VueltaCruce v;
    v.hubo_paquete = (paquete != nullptr);
    v.cambio       = CambioPaquete::SIN_CAMBIOS;
    v.muestreo     = false;

    // 1. Paquete nuevo: sólo los del semáforo seguido mantienen vivo el temporizador
    if (paquete != nullptr) {
        v.cambio = evaluador_.registrarPaquete(*paquete);
        if (v.cambio != CambioPaquete::IGNORADO) {
            ultimo_paquete_ = ahora;
        }
    }

    // 2. Cerrar el patrón que ya terminó, para que el muestreo no se salte
    //    justo la vuelta en que los motores quedan quietos
    patrones_.actualizar(ahora);

    // 3. Orientación a 5 Hz, nunca con un motor vibrando ni justo después
    if ((uint32_t)(ahora - ultimo_muestreo_) >= INTERVALO_MUESTREO_MS &&
        patrones_.quietosDesde(ahora, ASENTAMIENTO_TRAS_VIBRAR_MS)) {
        ultimo_muestreo_ = ahora;
        v.muestreo = true;
        evaluador_.registrarOrientacion(leer(evaluador_.ultimoPaquete(), contexto));
    }

    // 4. Un patrón sólo empieza si hay un evento y el controlador está libre
    v.iniciado = evaluador_.orquestar(ahora);

    // 5. Los motores siguen exactamente lo que dice el controlador
    v.salida = patrones_.actualizar(ahora);
    return v;
}

bool CoordinadorCruce::probarVibracion(uint32_t ahora) {
    return patrones_.iniciar(Patron::SEMAFORO_DETECTADO, ahora);
}

void CoordinadorCruce::detenerVibracion() {
    patrones_.cancelar();
}

bool CoordinadorCruce::sinPaquetes(uint32_t ahora) const {
    return (uint32_t)(ahora - ultimo_paquete_) >= TIEMPO_SIN_PAQUETES_STANDBY_MS;
}
