#include "semaforo.h"
//Incluimos las bibliotecas necesarias para ESP-Now
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "../../control/fsm.h"

//Inicializamos los atributos de la clase
const uint8_t ComunicadorSemaforo::BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
estado_semaforo ComunicadorSemaforo::ultimoEstado = {};

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
    if (esPaqueteSemaforo(data, data_len)) {
        memcpy(&ultimoEstado, data, sizeof(estado_semaforo));
        MaquinaEstados::registrarPaqueteRecibido();
    }
}


//Desarrollamos el contenido del metodo para inicializar el dispositivo
bool ComunicadorSemaforo::inicializar() {
    WiFi.mode(WIFI_STA);
    //Evaluamos para determinar si la inicializacion de ESP-Now fue exitosa
    if (esp_now_init() != ESP_OK) {
        return false;
    }
    //Evaluamos para determinar si el registro del callback es exitoso
    if (esp_now_register_recv_cb(onDatosRecibidos) != ESP_OK) {
        return false;
    }
    //Retornamos verdadero si la comunicacion se ha establecido correctamente
    return true;
}

//Desarrollamos el contenido para desinicializar el dispositivo
bool ComunicadorSemaforo::desinicializar() {
    //Evaluamos si se eliminó el registro del callback correctamente
    if (esp_now_unregister_recv_cb() != ESP_OK) {
        return false;
    }
    //Evaluamos si se apago correctamente ESP-Now
    if (esp_now_deinit() != ESP_OK) {
        return false;
    }
    //Retornamos verdadero una vez que haya salido correctamente
    return true;
}

