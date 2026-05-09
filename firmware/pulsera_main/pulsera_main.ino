// Archivo: pulsera_integrada_simplificada.ino

// --- LIBRERÍAS ---
#include <esp_now.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "estructura.h"
#include <esp_wifi.h>

// --- CONFIGURACIÓN HARDWARE ---
const int MOTOR_PIN = 16;

#define SERVICE_UUID      "12345678-1234-1234-1234-123456789abc"
#define TX_CHAR_UUID      "87654321-4321-4321-4321-cba987654321" // Notificación (ESP32 -> App)
#define RX_CHAR_UUID      "11111111-2222-3333-4444-555555555555" // Escritura (App -> ESP32)

// --- VARIABLES GLOBALES ---
volatile int semaforoEstado = 2;
volatile int semaforoAngulo = 0;
volatile bool nuevosDatosEspNow = false;
volatile bool isVibrating = false;

// --- NUEVO: Variables para la vibración sin bloqueo ---
unsigned long lastVibrationTime = 0;
const int vibrationInterval = 200; // Intervalo de pulso en ms
int motorState = LOW;

BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL;
BLECharacteristic* pRxCharacteristic = NULL;
bool bleConnected = false;

// --- CALLBACKS DE BLE ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        bleConnected = true;
        Serial.println("BLE: Cliente Conectado");
    }

    void onDisconnect(BLEServer* pServer) {
        bleConnected = false;
        isVibrating = false;
        digitalWrite(MOTOR_PIN, LOW); // Apagar motor por seguridad
        Serial.println("BLE: Cliente Desconectado, reiniciando advertising...");
        pServer->getAdvertising()->start();
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            Serial.print("BLE: Comando recibido de la App: ");
            Serial.println(rxValue);

            if (rxValue == "VIBRATE_START") {
                isVibrating = true;
            } else if (rxValue == "VIBRATE_STOP") {
                isVibrating = false;
            }
        }
    }
};


// --- CALLBACK DE ESP-NOW ---
void OnDataRecv(const esp_now_recv_info * info, const uint8_t *incomingData, int len) {
  esp_now_data_structure datosRecibidos;
  memcpy(&datosRecibidos, incomingData, sizeof(datosRecibidos));

  semaforoEstado = datosRecibidos.estado;
  semaforoAngulo = datosRecibidos.angulo;
  nuevosDatosEspNow = true;

  Serial.println("---------------------------------");
  Serial.println("ESP-NOW: ¡Datos recibidos!");
  Serial.printf("Estado: %d, Ángulo: %d\n", semaforoEstado, semaforoAngulo);
  Serial.println("---------------------------------");
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Pulsera Inteligente (Modo Coexistencia)...");

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  WiFi.mode(WIFI_STA);
  BLEDevice::init("ESP32_Cerebro_IoT");

  if (esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR) != ESP_OK) {
    Serial.println("Error al establecer el protocolo WiFi para coexistencia");
    return;
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW: Listo para recibir datos.");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(TX_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  pRxCharacteristic = pService->createCharacteristic(RX_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("BLE: Esperando conexión de la app...");
}

// --- LOOP PRINCIPAL ---
void loop() {
  if (nuevosDatosEspNow) {
    nuevosDatosEspNow = false;

    if (bleConnected) {
        Serial.println("BLE: Enviando datos del semáforo a la app...");
        char msg[32];
        sprintf(msg, "%d,%d", semaforoEstado, semaforoAngulo);
        pTxCharacteristic->setValue(msg);
        pTxCharacteristic->notify();
    }
  }

  // --- Lógica de vibración SIN BLOQUEO ---
  unsigned long currentTime = millis();

  if (isVibrating) {
    // Comprueba si ha pasado el tiempo para cambiar el estado del motor
    if (currentTime - lastVibrationTime >= vibrationInterval) {
      lastVibrationTime = currentTime; // Reinicia el cronómetro

      // Cambia el estado del motor (de HIGH a LOW o viceversa)
      if (motorState == LOW) {
        motorState = HIGH;
      } else {
        motorState = LOW;
      }
      digitalWrite(MOTOR_PIN, motorState);
    }
  } else {
    // Si no debe vibrar, asegurarse de que el motor esté apagado.
    if (motorState == HIGH) {
      motorState = LOW;
      digitalWrite(MOTOR_PIN, LOW);
    }
  }
}