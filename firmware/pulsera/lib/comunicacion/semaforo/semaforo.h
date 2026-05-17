#ifndef COMUNICACION_SEMAFORO_H
#define COMUNICACION_SEMAFORO_H

#include <Arduino.h>
#include <stdint.h> //Incluimos para usar enteros de un byte
#include <WiFi.h>
#include <esp_now.h>
#include "../protocolo/protocolo_esp.h" //Incluimos para usar la estructura de comunicacion

class ComunicadorSemaforo {
private:
    static const uint8_t BROADCAST_MAC[6]; //Direccion MAC
public:
    static estado_semaforo ultimoEstado; //Ultimo paquete de datos recibido
    static bool inicializar(); //Establece la comunicacion con el Semaforo
    static bool desinicializar(); //Detiene la comunicacoin con el Semaforo
private:
    static void onDatosRecibidos(const uint8_t *mac_addr, const uint8_t *data, int data_len); //

};

#endif