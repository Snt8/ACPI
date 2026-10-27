#include "movil.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Arduino.h>

// Se usa la biblioteca BLE que ya trae el core Arduino-ESP32, así no hace falta
// instalar nada aparte. En el ESP32-C3 con core v3.x corre sobre NimBLE (el mismo
// stack ligero que antes se traía con NimBLE-Arduino).
#if defined(CONFIG_BLUEDROID_ENABLED)
#include <BLE2902.h>  // Bluedroid necesita el descriptor CCCD explícito para notify
#endif

#define SERVICE_UUID           "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID_TX "87654321-4321-4321-4321-cba987654321" // Notify
#define CHARACTERISTIC_UUID_RX "11111111-2222-3333-4444-555555555555" // Write

static BLEServer* pServer = nullptr;
static BLECharacteristic* pTxCharacteristic = nullptr;
static BLECharacteristic* pRxCharacteristic = nullptr;
static bool clienteConectado = false;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        clienteConectado = true;
        Serial.println("[BLE] Cliente conectado!");
    }

    void onDisconnect(BLEServer* pServer) override {
        clienteConectado = false;
        Serial.println("[BLE] Cliente desconectado. Reiniciando publicidad...");
        BLEDevice::startAdvertising();
    }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        // String en core v3.x, std::string en v2.x: ambos tienen length() y []
        auto value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.print("[BLE RX] Datos recibidos: ");
            for (size_t i = 0; i < value.length(); i++) {
                Serial.print(value[i]);
            }
            Serial.println();
        }
    }
};

// Los callbacks son estáticos: el BLE se enciende y apaga en cada ciclo de
// standby, y crearlos con new en cada encendido iría perdiendo memoria
static MyServerCallbacks         callbacksServidor;
static MyCharacteristicCallbacks callbacksRx;

bool ComunicadorMovil::inicializar() {
    if (pServer != nullptr) {
        return true;  // ya estaba encendido
    }
    if (!BLEDevice::init("ACPI_Brazalete")) {
        return false;
    }

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(&callbacksServidor);

    BLEService* pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
#if defined(CONFIG_BLUEDROID_ENABLED)
    pTxCharacteristic->addDescriptor(new BLE2902());
#endif

    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );
    pRxCharacteristic->setCallbacks(&callbacksRx);

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();

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
        // deinit(false): apaga el stack y la radio BT pero conserva la memoria
        // del controlador, para poder volver a encenderlo al salir de standby
        // (con true, BLEDevice::init() fallaría para siempre)
        BLEDevice::deinit(false);
        pServer = nullptr;
        pTxCharacteristic = nullptr;
        pRxCharacteristic = nullptr;
        clienteConectado = false;
        Serial.println("[BLE] Servidor BLE desinicializado.");
    }
}
