#include "movil.h"
#include <NimBLEDevice.h>
#include <Arduino.h>

#define SERVICE_UUID           "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID_TX "87654321-4321-4321-4321-cba987654321" // Notify
#define CHARACTERISTIC_UUID_RX "11111111-2222-3333-4444-555555555555" // Write

static NimBLEServer* pServer = nullptr;
static NimBLECharacteristic* pTxCharacteristic = nullptr;
static NimBLECharacteristic* pRxCharacteristic = nullptr;
static bool clienteConectado = false;

class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        clienteConectado = true;
        Serial.println("[BLE] Cliente conectado!");
    }
    
    void onDisconnect(NimBLEServer* pServer) override {
        clienteConectado = false;
        Serial.println("[BLE] Cliente desconectado. Reiniciando publicidad...");
        NimBLEDevice::startAdvertising();
    }
};

class MyCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.print("[BLE RX] Datos recibidos: ");
            for (size_t i = 0; i < value.length(); i++) {
                Serial.print(value[i]);
            }
            Serial.println();
        }
    }
};

bool ComunicadorMovil::inicializar() {
    NimBLEDevice::init("ACPI_Pulsera");
    
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY
    );
    
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE
    );
    pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    
    pService->start();
    
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    
    Serial.println("[BLE] Servidor BLE inicializado correctamente.");
    return true;
}

bool ComunicadorMovil::estaConectado() {
    return clienteConectado;
}

void ComunicadorMovil::enviarDatos(uint16_t heading, uint8_t estado, bool orientacionOk, uint8_t tiempoRestante, int8_t bateria) {
    if (!clienteConectado || pTxCharacteristic == nullptr) {
        return;
    }
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), 
             "{\"hd\":%u,\"st\":%u,\"ok\":%s,\"tr\":%u,\"bt\":%d}",
             heading, estado, orientacionOk ? "true" : "false", tiempoRestante, (int)bateria);
             
    pTxCharacteristic->setValue((uint8_t*)buffer, strlen(buffer));
    pTxCharacteristic->notify();
}

void ComunicadorMovil::desinicializar() {
    if (pServer != nullptr) {
        NimBLEDevice::deinit(true);
        pServer = nullptr;
        pTxCharacteristic = nullptr;
        pRxCharacteristic = nullptr;
        clienteConectado = false;
        Serial.println("[BLE] Servidor BLE desinicializado.");
    }
}
