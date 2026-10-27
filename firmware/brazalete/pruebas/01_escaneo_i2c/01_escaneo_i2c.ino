// ── Prueba 01: escaneo del bus I2C ───────────────────────────────────────────
// Recorre todas las direcciones 0x01–0x7E y muestra cuáles responden, con el
// sensor que suele usar cada una. Repite cada 5 s, así se puede abrir el monitor
// serie en cualquier momento o mover un cable y ver el cambio.
//
// Antes de escanear revisa que SDA y SCL estén en alto en reposo: si alguna
// línea está en bajo, falta la resistencia pull-up o hay un corto a GND.
//
// Arduino IDE: placa "ESP32C3 Dev Module", "USB CDC On Boot: Enabled",
// monitor serie a 115200 baudios.

#include <Wire.h>

#define BAUD_RATE             115200
#define PIN_I2C_SDA           8
#define PIN_I2C_SCL           9
#define INTERVALO_ESCANEO_MS  5000UL

// Sensor que suele estar en cada dirección (la confirmación real la hace
// la prueba 02_identificar_sensores leyendo el registro de ID del chip)
const char* nombreProbable(uint8_t direccion) {
    switch (direccion) {
        case 0x0C: return "AK8963 (magnetometro dentro del MPU9250, solo visible con bypass)";
        case 0x0D: return "QMC5883L (GY-271 version QST)";
        case 0x1E: return "HMC5883L (GY-271 version Honeywell)";
        case 0x2C: return "QMC5883P o QMC6308";
        case 0x68: return "MPU6050 / MPU6500 / MPU9250 (AD0 a GND)";
        case 0x69: return "MPU6050 / MPU6500 / MPU9250 (AD0 a VCC)";
        default:   return "desconocido";
    }
}

// Con el bus en reposo y sin pull-up interno, ambas líneas deben leer 1
void revisarLineas() {
    pinMode(PIN_I2C_SDA, INPUT);
    pinMode(PIN_I2C_SCL, INPUT);
    delay(2);
    int sda = digitalRead(PIN_I2C_SDA);
    int scl = digitalRead(PIN_I2C_SCL);
    Serial.printf("[LINEAS] SDA (GPIO%d) = %s | SCL (GPIO%d) = %s\n",
                  PIN_I2C_SDA, sda ? "ALTO" : "BAJO",
                  PIN_I2C_SCL, scl ? "ALTO" : "BAJO");
    if (!sda || !scl) {
        Serial.println(F("[LINEAS] AVISO: una linea esta en bajo -> falta pull-up, "
                         "el modulo no tiene alimentacion o hay un corto a GND."));
    }
}

void escanear() {
    Serial.println(F("\n[I2C] Escaneando 0x01-0x7E..."));
    uint8_t encontrados = 0;

    for (uint8_t direccion = 0x01; direccion < 0x7F; direccion++) {
        Wire.beginTransmission(direccion);
        uint8_t resultado = Wire.endTransmission();

        if (resultado == 0) {
            Serial.printf("[I2C]   0x%02X -> %s\n", direccion, nombreProbable(direccion));
            encontrados++;
        } else if (resultado == 4) {
            Serial.printf("[I2C]   0x%02X -> error de bus (4)\n", direccion);
        }
    }

    if (encontrados == 0) {
        Serial.println(F("[I2C] No respondio ningun dispositivo. Revisar VCC, GND, SDA y SCL."));
    } else {
        Serial.printf("[I2C] %u dispositivo(s) encontrado(s).\n", encontrados);
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    delay(1000);
    Serial.println(F("\n=== ACPI | Prueba 01: escaneo I2C ==="));
}

void loop() {
    revisarLineas();
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    escanear();
    Wire.end();  // libera los pines para revisarLineas() en la siguiente vuelta

    delay(INTERVALO_ESCANEO_MS);
}
