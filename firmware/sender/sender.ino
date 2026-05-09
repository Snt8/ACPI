#include <esp_now.h>
#include <WiFi.h>
#include "estructura.h" // Incluimos la estructura de datos compartida
#include "const.h"      // Archivo con la dirección MAC del receptor

// --- CONFIGURACIÓN DEL SEMÁFORO ---
const int ldrPin = 34;
const int UMBRAL_LUZ = 1200; // Ajusta este valor según tu LDR y la luz ambiente
const int anguloSemaforo = 90; // ¡IMPORTANTE! Define el ángulo de este semáforo (0, 90, 180, 270)

// Variable para almacenar los datos a enviar
esp_now_data_structure datosSemaforo;

// Modifica la firma de la función OnDataSent
// Ahora el primer argumento debe ser const wifi_tx_info_t*
void OnDataSent(const wifi_tx_info_t *txInfo, esp_now_send_status_t status) {
  Serial.print("\r\nEstado del último envío: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Entrega Correcta" : "Fallo en la Entrega");
}

void setup() {
  Serial.begin(115200);
  
  // Configurar el modo WiFi
  WiFi.mode(WIFI_STA);

  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  // Registrar el callback de envío
  esp_now_register_send_cb(OnDataSent);
  
  // Registrar el peer (receptor)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, MAC_RECEIVER_1, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Fallo al añadir el peer");
    return;
  }
  Serial.println("Peer añadido correctamente.");
}

void loop() {
  // Leer el sensor de luz para determinar el estado del semáforo
  int ldrValue = analogRead(ldrPin);
  Serial.print(ldrValue);
  // Asignar estado: 1 para VERDE (luz detectada), 2 para ROJO (sin luz)
  datosSemaforo.estado = (ldrValue > UMBRAL_LUZ) ? 1 : 2;
  
  // Asignar el ángulo configurado
  datosSemaforo.angulo = anguloSemaforo;

  // Enviar la estructura de datos al receptor
  esp_err_t result = esp_now_send(MAC_RECEIVER_1, (uint8_t *) &datosSemaforo, sizeof(datosSemaforo));
   
  if (result == ESP_OK) {
    Serial.printf("Enviado con éxito -> Estado: %d, Ángulo: %d\n", datosSemaforo.estado, datosSemaforo.angulo);
  } else {
    Serial.println("Error al enviar los datos");
  }

  delay(2000); // Enviar datos cada 2 segundos
}
