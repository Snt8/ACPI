#ifndef COMUNICACION_SEMAFORO_H
#define COMUNICACION_SEMAFORO_H

#include <Arduino.h>
#include <stdint.h> //Incluimos para usar enteros de un byte
#include <esp_idf_version.h>
#include <WiFi.h>
#include <esp_now.h>
#include "../../protocolo/protocolo_esp.h" //Incluimos el protocolo de ESP-Now para usar la estructura de la comunicacion

class ComunicadorSemaforo {
private:
    static const uint8_t BROADCAST_MAC[6]; //Direccion MAC
public:
    static estado_semaforo ultimoEstado; //Ultimo paquete de datos recibido
    static bool inicializar(); //Establece la comunicacion con el Semaforo
    static bool desinicializar(); //Detiene la comunicacoin con el Semaforo
private:
    // La firma del callback cambió en ESP-IDF 5 (core Arduino-ESP32 v3.x): ahora recibe
    // esp_now_recv_info_t en vez de la MAC remota. Se compila la que corresponda.
#if ESP_IDF_VERSION_MAJOR >= 5
    static void onDatosRecibidos(const esp_now_recv_info_t *info, const uint8_t *data, int data_len); //Callback que convierte los datos a la estructura
#else
    static void onDatosRecibidos(const uint8_t *mac_addr, const uint8_t *data, int data_len); //Callback que convierte los datos a la estructura
#endif

};

#endif
