#include "semaforo.h"
//Incluimos las bibliotecas necesarias para ESP-Now
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

//Inicializamos los atributos de la clase
const uint8_t ComunicadorSemaforo::BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
estado_semaforo ComunicadorSemaforo::ultimoEstado = {};

// Buffer compartido entre el callback (tarea de WiFi) y loop(). Se protege con
// una sección crítica; sólo guarda el último paquete porque el anterior ya no
// sirve si llegó uno más nuevo.
static portMUX_TYPE     mux_paquete      = portMUX_INITIALIZER_UNLOCKED;
static estado_semaforo  paquete_pendiente = {};
static bool             hay_pendiente     = false;
static uint32_t         descartados       = 0;

//Desarrollamos el metodo para pasar la callback con la información
#if ESP_IDF_VERSION_MAJOR >= 5
void ComunicadorSemaforo::onDatosRecibidos(const esp_now_recv_info_t *info, const uint8_t *data, int data_len) {
    (void)info; // Trae la MAC remota; no es necesaria en este manejador actualmente
#else
void ComunicadorSemaforo::onDatosRecibidos(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    (void)mac_addr; // La MAC remota se recibe aquí; no es necesaria en este manejador actualmente
#endif
    // Sólo se aceptan paquetes del semáforo ACPI (tamaño exacto y firma "ACPI");
    // cualquier otro broadcast ESP-NOW del canal se ignora
    portENTER_CRITICAL(&mux_paquete);
    if (esPaqueteSemaforo(data, data_len)) {
        memcpy(&paquete_pendiente, data, sizeof(estado_semaforo));
        hay_pendiente = true;
    } else {
        descartados++;
    }
    portEXIT_CRITICAL(&mux_paquete);
}


//true mientras ESP-NOW esté escuchando. El standby enciende y apaga la radio
//cada 5 s; con esta marca, llamar dos veces seguidas no hace nada
static bool radio_activa = false;

//Desarrollamos el contenido del metodo para inicializar el dispositivo
bool ComunicadorSemaforo::inicializar() {
    if (radio_activa) {
        return true;
    }
    WiFi.mode(WIFI_STA);
    //Evaluamos para determinar si la inicializacion de ESP-Now fue exitosa
    if (esp_now_init() != ESP_OK) {
        WiFi.mode(WIFI_OFF);
        return false;
    }
    //Evaluamos para determinar si el registro del callback es exitoso
    if (esp_now_register_recv_cb(onDatosRecibidos) != ESP_OK) {
        esp_now_deinit();
        WiFi.mode(WIFI_OFF);
        return false;
    }
    radio_activa = true;
    //Retornamos verdadero si la comunicacion se ha establecido correctamente
    return true;
}

//Desarrollamos el contenido para desinicializar el dispositivo
bool ComunicadorSemaforo::desinicializar() {
    bool ok = true;
    if (radio_activa) {
        //Se intenta todo aunque un paso falle: lo importante es que la radio quede apagada
        ok &= (esp_now_unregister_recv_cb() == ESP_OK);
        ok &= (esp_now_deinit() == ESP_OK);
        radio_activa = false;
    }
    //Apagamos el WiFi por completo (en standby la radio sólo se enciende para escuchar)
    ok &= WiFi.mode(WIFI_OFF);
    return ok;
}

bool ComunicadorSemaforo::tomarPaquete(estado_semaforo& paquete) {
    bool habia = false;
    portENTER_CRITICAL(&mux_paquete);
    if (hay_pendiente) {
        paquete = paquete_pendiente;
        hay_pendiente = false;
        habia = true;
    }
    portEXIT_CRITICAL(&mux_paquete);
    if (habia) {
        ultimoEstado = paquete;
    }
    return habia;
}

bool ComunicadorSemaforo::hayPaquete() {
    portENTER_CRITICAL(&mux_paquete);
    bool habia = hay_pendiente;
    portEXIT_CRITICAL(&mux_paquete);
    return habia;
}

void ComunicadorSemaforo::descartarPendiente() {
    portENTER_CRITICAL(&mux_paquete);
    hay_pendiente = false;
    portEXIT_CRITICAL(&mux_paquete);
}

uint32_t ComunicadorSemaforo::paquetesDescartados() {
    portENTER_CRITICAL(&mux_paquete);
    uint32_t total = descartados;
    portEXIT_CRITICAL(&mux_paquete);
    return total;
}
