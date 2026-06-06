#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[SEMAFORO] Iniciando...");
}

void loop() {
    Serial.println("Funcionando");
    delay(5000);
}