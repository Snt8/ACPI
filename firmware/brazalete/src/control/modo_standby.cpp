#include "modo_standby.h"
#include "ciclo_escucha.h"
#include "../comunicacion/semaforo/semaforo.h"
#include "../config.h"
#include "../constantes.h"
#include <Arduino.h>
#include <esp_sleep.h>

static CicloEscucha ciclo;

// true si hay un cable USB con datos conectado. Con "USB CDC On Boot" el
// Serial es el USB nativo del C3; el light sleep lo desconectaría, así que
// mientras esté enchufado (depuración) se espera con delay() en su lugar.
static bool usbConectado() {
#if ARDUINO_USB_MODE && ARDUINO_USB_CDC_ON_BOOT
    return HWCDC::isPlugged();
#else
    return false;
#endif
}

void ModoStandby::entrar(uint32_t ahora, bool escucharYa) {
    ComunicadorSemaforo::desinicializar();  // WiFi apagado hasta la primera ventana
    ComunicadorSemaforo::descartarPendiente();
    ciclo.iniciar(ahora, escucharYa);
    Serial.printf("[STANDBY] Radio apagada; escucha de %lu ms cada %lu ms\n",
                  (unsigned long)VENTANA_ESCUCHA_STANDBY_MS,
                  (unsigned long)INTERVALO_ESCUCHA_STANDBY_MS);
}

bool ModoStandby::actualizar(uint32_t ahora) {
    // Un paquete que llegó durante la ventana se atiende antes de cerrarla
    if (ciclo.escuchando() && ComunicadorSemaforo::hayPaquete()) {
        Serial.println(F("[STANDBY] Paquete del semaforo recibido: saliendo de standby"));
        return true;  // la radio queda encendida para SENSING_CROSSING
    }

    switch (ciclo.actualizar(ahora)) {
        case CicloEscucha::Accion::ENCENDER_RADIO:
            if (!ComunicadorSemaforo::inicializar()) {
                Serial.println(F("[STANDBY] [ERROR] No se pudo encender ESP-NOW"));
            }
            break;
        case CicloEscucha::Accion::APAGAR_RADIO:
            ComunicadorSemaforo::desinicializar();
            break;
        case CicloEscucha::Accion::NADA:
            break;
    }

    if (ciclo.escuchando()) {
        delay(10);  // cede la CPU mientras la radio escucha
    } else {
        esperar(ciclo.msHastaProximaEscucha(millis()));
    }
    return false;
}

void ModoStandby::esperar(uint32_t ms) {
    if (ms == 0) return;
#if STANDBY_LIGHT_SLEEP
    if (!usbConectado()) {
        Serial.flush();
        esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000ULL);
        esp_light_sleep_start();  // millis() sigue contando durante el sueño
        return;
    }
#endif
    // Con USB conectado (o sin light sleep) se espera en tramos cortos para
    // que el heartbeat de brazalete.ino siga saliendo por el monitor serie
    delay(ms < 100 ? ms : 100);
}
