#ifndef COMUNICADOR_PULSERA_H
#define COMUNICADOR_PULSERA_H

#include <Arduino.h>
#include <stdint.h>
#include <WiFi.h>
#include <esp_now.h>
#include "protocolo_esp.h" // Estructura de comunicación compartida

class ComunicadorPulsera {
private:
    static const uint8_t BROADCAST_MAC[6];
    static esp_now_peer_info_t peerInfo;

public:
    static bool inicializar();
    static bool enviarEstado(const estado_semaforo& estado);
    static bool desinicializar();
};

#endif // COMUNICADOR_PULSERA_H
