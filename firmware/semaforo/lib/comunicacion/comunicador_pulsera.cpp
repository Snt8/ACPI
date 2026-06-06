#include "comunicador_pulsera.h"
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

const uint8_t ComunicadorPulsera::BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t ComunicadorPulsera::peerInfo = {};

bool ComunicadorPulsera::inicializar() {
    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) {
        return false;
    }
    
    // Configurar el peer de broadcast
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
    peerInfo.channel = 0;  // Cualquier canal
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        return false;
    }
    
    return true;
}

bool ComunicadorPulsera::enviarEstado(const estado_semaforo& estado) {
    esp_err_t resultado = esp_now_send(BROADCAST_MAC, (const uint8_t*)&estado, sizeof(estado));
    return (resultado == ESP_OK);
}

bool ComunicadorPulsera::desinicializar() {
    if (esp_now_del_peer(BROADCAST_MAC) != ESP_OK) {
        // Ignorar error de peer inexistente si ya se desinicializó
    }
    if (esp_now_deinit() != ESP_OK) {
        return false;
    }
    return true;
}
