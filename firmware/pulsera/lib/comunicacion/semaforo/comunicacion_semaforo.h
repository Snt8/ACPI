#ifndef COMUNICACION_SEMAFORO_H
#define COMUNICACION_SEMAFORO_H

//Incluimos la libreria de Arduino
#include <Arduino.h>

//Incluimos las librerias para esp_now
#include <esp_now.h>
#include <WiFi.h>

inline void iniciarDispositivo() {
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        // TODO: manejar el error de inicialización.
    }
}

#endif