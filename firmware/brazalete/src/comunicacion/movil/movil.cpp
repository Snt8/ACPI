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

// Cuánto se espera a que el móvil confirme la desconexión antes de apagar el stack
#define ESPERA_DESCONEXION_MS 500

static BLEServer* pServer = nullptr;
static BLEService* pService = nullptr;
static BLECharacteristic* pTxCharacteristic = nullptr;
static BLECharacteristic* pRxCharacteristic = nullptr;
// Se escriben desde la tarea de NimBLE y se leen desde loop()
static volatile bool clienteConectado = false;
static volatile bool apagando = false;  // desinicializar() en curso

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        clienteConectado = true;
        Serial.println("[BLE] Cliente conectado!");
    }

    void onDisconnect(BLEServer* pServer) override {
        clienteConectado = false;
        if (apagando) {
            return;  // se está apagando el BLE: no volver a anunciarse
        }
        Serial.println("[BLE] Cliente desconectado. Reiniciando publicidad...");
        BLEDevice::startAdvertising();
    }
};

// Lo que escribe el móvil sólo se muestra por el monitor serie. Para que un
// equipo cercano que escriba sin parar no sature el Serial (y con él la tarea
// de NimBLE), se muestran como mucho MAX_RX_MOSTRAR bytes y un mensaje cada
// INTERVALO_RX_MIN_MS; el resto se ignora.
#define MAX_RX_MOSTRAR       32
#define INTERVALO_RX_MIN_MS  200

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        static unsigned long ultimo_rx = 0;
        unsigned long ahora = millis();
        if (ahora - ultimo_rx < INTERVALO_RX_MIN_MS) {
            return;
        }
        ultimo_rx = ahora;
        // String en core v3.x, std::string en v2.x: ambos tienen length() y []
        auto value = pCharacteristic->getValue();
        size_t largo = value.length();
        if (largo > 0) {
            Serial.print("[BLE RX] Datos recibidos: ");
            for (size_t i = 0; i < largo && i < MAX_RX_MOSTRAR; i++) {
                Serial.print(value[i]);
            }
            Serial.println(largo > MAX_RX_MOSTRAR ? "..." : "");
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

    pService = pServer->createService(SERVICE_UUID);

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
    if (pServer == nullptr) {
        return;
    }
    apagando = true;
    BLEDevice::stopAdvertising();

    // Con el móvil conectado hay que cortar la conexión ANTES de apagar el
    // stack: BLEDevice::deinit() borra el servidor y recién después detiene
    // NimBLE, cuyo evento de desconexión llegaría entonces a un servidor ya
    // liberado (y onDisconnect volvería a anunciarse en pleno apagado)
    if (clienteConectado) {
        pServer->disconnect(pServer->getConnId());
        unsigned long inicio = millis();
        while (clienteConectado && millis() - inicio < ESPERA_DESCONEXION_MS) {
            delay(10);
        }
    }

    // deinit(false): apaga el stack y la radio BT pero conserva la memoria
    // del controlador, para poder volver a encenderlo al salir de standby
    // (con true, BLEDevice::init() fallaría para siempre)
    BLEDevice::deinit(false);

    // deinit() borra el servidor, pero no el servicio ni sus características.
    // Las características se liberan aquí; el servicio no se puede (la
    // biblioteca declara privado su destructor), así que cada ciclo de standby
    // pierde unos cientos de bytes. El heartbeat muestra el heap libre para
    // vigilarlo en la placa.
    delete pTxCharacteristic;
    delete pRxCharacteristic;
    pServer = nullptr;
    pService = nullptr;
    pTxCharacteristic = nullptr;
    pRxCharacteristic = nullptr;
    clienteConectado = false;
    apagando = false;
    Serial.println("[BLE] Servidor BLE desinicializado.");
}
