// ── Prueba 04: diagnóstico del bus I2C ───────────────────────────────────────
// Para cuando el escaneo (prueba 01) dice que responden TODAS las direcciones.
// Eso significa que SDA está en bajo justo cuando el ESP32 espera la respuesta
// (ACK) de un sensor. Puede ser:
//   a) un sensor dañado o trabado que baja SDA a cualquier dirección;
//   b) una línea que sube demasiado lento (fuga o capacidad de más), aunque en
//      reposo el multímetro marque bien.
// Este sketch maneja el bus "a mano" (sin la biblioteca Wire) para distinguirlo:
//   1. Mide cuánto tardan SDA y SCL en subir solas por sus pull-ups.
//   2. Libera un sensor trabado con 9 pulsos de reloj + STOP.
//   3. Pregunta por direcciones que existen (0x0D, 0x68) y que no (0x50, 0x23)
//      a 10 kHz, y mira si alguien baja SDA cuando el ESP32 la tiene suelta.
//   4. Escanea con Wire a 100 kHz y a 10 kHz y lee el WHO_AM_I del MPU.
// No toca los pines de los motores. Repite cada 10 s.
//
// Arduino IDE: placa "ESP32C3 Dev Module", "USB CDC On Boot: Enabled",
// monitor serie a 115200 baudios. Basta con el USB (sin batería).

#include <Wire.h>

#define BAUD_RATE      115200
#define PIN_SDA        8
#define PIN_SCL        9
#define MEDIO_BIT_US   50     // 10 kHz: muy lento a propósito
#define INTERVALO_MS   10000UL

// ── Bus "a mano": en I2C nadie empuja la línea a 1, sólo se suelta ──────────
void soltar(int pin) { pinMode(pin, INPUT); }   // la sube el pull-up
void bajar(int pin)  { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
bool alta(int pin)   { return digitalRead(pin) == HIGH; }
void esperar()       { delayMicroseconds(MEDIO_BIT_US); }

// Microsegundos que tarda la línea en volver a 1 tras soltarla (tope 2000)
uint32_t tiempoSubida(int pin) {
    bajar(pin);
    delayMicroseconds(20);
    uint32_t t0 = micros();
    soltar(pin);
    while (!alta(pin)) {
        if (micros() - t0 > 2000) return 2000;
    }
    return micros() - t0;
}

// 9 pulsos de reloj con SDA suelta y un STOP: si un sensor quedó a mitad de un
// envío, termina su byte y suelta el bus
void liberarBus() {
    soltar(PIN_SDA);
    for (int i = 0; i < 9; i++) {
        bajar(PIN_SCL); esperar();
        soltar(PIN_SCL); esperar();
    }
    bajar(PIN_SDA); esperar();
    soltar(PIN_SCL); esperar();
    soltar(PIN_SDA); esperar();
}

// Envía START + dirección (escritura) y lee el ACK. 'intrusos' cuenta los bits
// en que el ESP32 tenía SDA suelta (un 1) y aun así la leyó en 0: alguien más
// la estaba bajando.
bool sondear(uint8_t dir, int& intrusos) {
    intrusos = 0;
    soltar(PIN_SDA); soltar(PIN_SCL); esperar();
    if (!alta(PIN_SDA)) intrusos++;            // SDA ya estaba baja antes del START
    bajar(PIN_SDA); esperar();                 // START
    bajar(PIN_SCL); esperar();
    uint8_t byte = (uint8_t)(dir << 1);        // bit 0 = 0: escritura
    for (int i = 7; i >= 0; i--) {
        bool uno = byte & (1 << i);
        if (uno) soltar(PIN_SDA); else bajar(PIN_SDA);
        esperar();
        soltar(PIN_SCL); esperar();
        if (uno && !alta(PIN_SDA)) intrusos++;
        bajar(PIN_SCL);
    }
    soltar(PIN_SDA); esperar();                // el sensor responde aquí
    soltar(PIN_SCL); esperar();
    bool ack = !alta(PIN_SDA);
    bajar(PIN_SCL); esperar();
    bajar(PIN_SDA); esperar();                 // STOP
    soltar(PIN_SCL); esperar();
    soltar(PIN_SDA); esperar();
    return ack;
}

int escanearWire(uint32_t frecuencia) {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(frecuencia);
    int encontrados = 0;
    for (uint8_t dir = 0x01; dir < 0x7F; dir++) {
        Wire.beginTransmission(dir);
        if (Wire.endTransmission() == 0) encontrados++;
    }
    Wire.end();
    return encontrados;
}

void diagnosticar() {
    Serial.println(F("\n=== Diagnostico del bus I2C ==="));

    // 1. Subida de las líneas
    uint32_t sube_sda = tiempoSubida(PIN_SDA);
    uint32_t sube_scl = tiempoSubida(PIN_SCL);
    Serial.printf("[SUBIDA] SDA %lu us | SCL %lu us   (bien: < 5 us; 2000 = no sube)\n",
                  (unsigned long)sube_sda, (unsigned long)sube_scl);

    // 2. Liberar un sensor trabado
    liberarBus();
    Serial.printf("[LIBERAR] 9 pulsos + STOP. En reposo: SDA=%s SCL=%s\n",
                  alta(PIN_SDA) ? "ALTO" : "BAJO", alta(PIN_SCL) ? "ALTO" : "BAJO");

    // 3. Sondeo a mano a 10 kHz
    const uint8_t dirs[] = {0x0D, 0x68, 0x50, 0x23};
    const char* que[]    = {"QMC5883L (existe)", "MPU (existe)", "nadie (no existe)", "nadie (no existe)"};
    int falsos_ack = 0, intrusos_total = 0;
    for (int i = 0; i < 4; i++) {
        int intrusos;
        bool ack = sondear(dirs[i], intrusos);
        intrusos_total += intrusos;
        if (i >= 2 && ack) falsos_ack++;
        Serial.printf("[A MANO] 0x%02X %-18s -> %s   bits pisados por otro: %d\n",
                      dirs[i], que[i], ack ? "ACK" : "NACK", intrusos);
    }

    // 4. Escaneo con Wire a dos velocidades
    int a100 = escanearWire(100000);
    int a10  = escanearWire(10000);
    Serial.printf("[WIRE] Responden %d direcciones a 100 kHz y %d a 10 kHz (bien: 2)\n", a100, a10);

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(10000);
    Wire.beginTransmission(0x68);
    Wire.write(0x75);
    uint8_t id = 0xEE;
    if (Wire.endTransmission(false) == 0 && Wire.requestFrom((uint8_t)0x68, (uint8_t)1) == 1) {
        id = Wire.read();
    }
    Wire.end();
    Serial.printf("[MPU] WHO_AM_I = 0x%02X (bien: 0x70)\n", id);

    // Conclusión
    Serial.print(F("[CONCLUSION] "));
    if (sube_sda >= 2000 || sube_scl >= 2000) {
        Serial.println(F("una linea no sube sola: algo la mantiene en bajo (corto o sensor)."));
    } else if (sube_sda > 5 || sube_scl > 5) {
        Serial.println(F("las lineas suben lento: fuga o capacidad de mas (residuos, humedad)."));
    } else if (falsos_ack > 0 || intrusos_total > 0) {
        Serial.println(F("un sensor baja SDA aunque no le hablen: sensor danado o trabado."));
    } else if (a100 > 2 && a10 <= 2) {
        Serial.println(F("a 10 kHz funciona y a 100 kHz no: problema de velocidad del bus."));
    } else if (a100 <= 2 && a10 <= 2) {
        Serial.println(F("el bus responde bien ahora (los 9 pulsos pudieron destrabarlo)."));
    } else {
        Serial.println(F("falla tambien a mano y lento: pasame toda esta salida."));
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    delay(1000);
    Serial.println(F("\n=== ACPI | Prueba 04: diagnostico I2C ==="));
}

void loop() {
    diagnosticar();
    delay(INTERVALO_MS);
}
