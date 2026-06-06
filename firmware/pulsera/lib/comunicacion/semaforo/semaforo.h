#ifndef COMUNICACION_SEMAFORO_H
#define COMUNICACION_SEMAFORO_H

#include <Arduino.h>
#include <stdint.h> //Incluimos para usar enteros de un byte
#include <WiFi.h>
#include <esp_now.h>
#include "../lib/protocolo/protocolo_esp.h" //Incluimos el protocolo de ESP-Now para usar la estructura de la comunicacion

class ComunicadorSemaforo {
private:
    static const uint8_t BROADCAST_MAC[6]; //Direccion MAC
public:
    static estado_semaforo ultimoEstado; //Ultimo paquete de datos recibido
    static bool inicializar(); //Establece la comunicacion con el Semaforo
    static bool desinicializar(); //Detiene la comunicacoin con el Semaforo
private:
    static void onDatosRecibidos(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len); //Callback que convierte los datos a la estructura

};

#endif