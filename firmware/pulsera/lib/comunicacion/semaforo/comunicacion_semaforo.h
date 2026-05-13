#ifndef COMUNICACION_SEMAFORO_H
#define COMUNICACION_SEMAFORO_H

//Incluimos la libreria de Arduino
#include <Arduino.h>

//Incluimos las librerias para esp_now
#include <esp_now.h>
#include <WiFi.h>

inline void iniciarDispositivo() {
    //Establecemos la frecuencia del dispositivo
    Serial.begin(115200);
    //Encendemos el chip de WiFi
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error en la inicializacion de ESP-Now");
        return;
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {

}


#endif