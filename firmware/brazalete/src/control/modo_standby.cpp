#include "modo_standby.h"
#include "ciclo_escucha.h"
#include "../comunicacion/semaforo/semaforo.h"
#include "../config.h"
#include "../constantes.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

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

// En light sleep el ESP32-C3 aísla los GPIO y los deja flotando. Si la base del
// transistor de un motor queda al aire, el motor podría encenderse sin ningún
// evento; por eso, durante el sueño, los pines de los motores quedan como
// entrada con pull-down interno.
static void asegurarMotoresEnSueno() {
    const gpio_num_t pines[] = {(gpio_num_t)PIN_MOTOR_DERECHO, (gpio_num_t)PIN_MOTOR_IZQUIERDO};
    for (gpio_num_t pin : pines) {
        gpio_sleep_set_direction(pin, GPIO_MODE_INPUT);
        gpio_sleep_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
        gpio_sleep_sel_en(pin);
    }
}

void ModoStandby::entrar(uint32_t ahora, bool escucharYa) {
    asegurarMotoresEnSueno();
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
            // Un paquete que llegó justo al cerrar la ventana ya no sirve: si se
            // dejara, la próxima ventana saldría de standby con datos de 5 s atrás
            ComunicadorSemaforo::descartarPendiente();
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
