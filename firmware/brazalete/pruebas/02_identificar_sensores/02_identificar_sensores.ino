// ── Prueba 02: identificar los sensores ──────────────────────────────────────
// No basta con saber que "algo" responde en una dirección: el GY-271 puede traer
// HMC5883L, QMC5883L o QMC5883P, y muchos módulos vendidos como MPU9250 son en
// realidad un MPU6500 (sin magnetómetro). Este sketch lee el registro de ID de
// cada chip para saber exactamente qué hay soldado, y al final imprime las
// líneas que hay que copiar en src/config.h del firmware.
//
// Pasos:
//   1. Despierta el MPU y activa su modo bypass, para que el AK8963 interno
//      del MPU9250 (si existe) aparezca en el bus como 0x0C.
//   2. Escanea el bus y prueba el ID de cada dirección conocida.
//   3. Repite cada 10 s.
//
// Arduino IDE: placa "ESP32C3 Dev Module", "USB CDC On Boot: Enabled",
// monitor serie a 115200 baudios.

#include <Wire.h>

#define BAUD_RATE                115200
#define PIN_I2C_SDA              8
#define PIN_I2C_SCL              9
#define INTERVALO_IDENTIFICAR_MS 10000UL

// ── Registros del MPU (comunes a MPU6050 / MPU6500 / MPU9250) ────────────────
#define MPU_REG_INT_PIN_CFG  0x37  // bit1 = BYPASS_EN
#define MPU_REG_USER_CTRL    0x6A  // bit5 = I2C_MST_EN (debe estar en 0 para el bypass)
#define MPU_REG_PWR_MGMT_1   0x6B
#define MPU_REG_WHO_AM_I     0x75

// ── Utilidades I2C ───────────────────────────────────────────────────────────
bool responde(uint8_t dir) {
    Wire.beginTransmission(dir);
    return Wire.endTransmission() == 0;
}

bool escribirRegistro(uint8_t dir, uint8_t reg, uint8_t valor) {
    Wire.beginTransmission(dir);
    Wire.write(reg);
    Wire.write(valor);
    return Wire.endTransmission() == 0;
}

bool leerRegistros(uint8_t dir, uint8_t reg, uint8_t* buffer, uint8_t cantidad) {
    Wire.beginTransmission(dir);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(dir, cantidad) != cantidad) return false;
    for (uint8_t i = 0; i < cantidad; i++) buffer[i] = Wire.read();
    return true;
}

// ── Resultado de la identificación ───────────────────────────────────────────
uint8_t dir_acelerometro  = 0;
uint8_t dir_magnetometro  = 0;
const char* chip_acelerometro = nullptr;
const char* chip_magnetometro = nullptr;

void registrarMagnetometro(uint8_t dir, const char* chip) {
    // Se sugiere para config.h el magnetómetro del GY-271; el AK8963 interno
    // del MPU9250 sólo se sugiere si no hay otro
    if (dir_magnetometro == 0 || dir_magnetometro == 0x0C) {
        dir_magnetometro  = dir;
        chip_magnetometro = chip;
    }
}

// ── Identificación por dirección ─────────────────────────────────────────────
void identificarMPU(uint8_t dir) {
    uint8_t id = 0;
    if (!leerRegistros(dir, MPU_REG_WHO_AM_I, &id, 1)) {
        Serial.printf("  0x%02X  no se pudo leer WHO_AM_I\n", dir);
        return;
    }

    const char* chip;
    switch (id) {
        case 0x68: chip = "MPU6050"; break;
        case 0x70: chip = "MPU6500 (sin magnetometro: NO es un MPU9250)"; break;
        case 0x71: chip = "MPU9250"; break;
        case 0x73: chip = "MPU9255"; break;
        default:   chip = "compatible MPU desconocido / clon"; break;
    }
    Serial.printf("  0x%02X  WHO_AM_I = 0x%02X -> %s\n", dir, id, chip);

    if (dir_acelerometro == 0) {
        dir_acelerometro  = dir;
        chip_acelerometro = chip;
    }
}

void identificarAK8963(uint8_t dir) {
    uint8_t id = 0;
    if (leerRegistros(dir, 0x00, &id, 1) && id == 0x48) {
        Serial.printf("  0x%02X  WIA = 0x48 -> AK8963 (magnetometro del MPU9250)\n", dir);
        registrarMagnetometro(dir, "AK8963");
    } else {
        Serial.printf("  0x%02X  responde, pero no es AK8963\n", dir);
    }
}

void identificarQMC5883L(uint8_t dir) {
    uint8_t id = 0;
    if (leerRegistros(dir, 0x0D, &id, 1) && id == 0xFF) {
        Serial.printf("  0x%02X  ID (reg 0x0D) = 0xFF -> QMC5883L\n", dir);
    } else {
        // Algunos clones no implementan el registro de ID; la dirección ya lo delata
        Serial.printf("  0x%02X  ID (reg 0x0D) = 0x%02X -> probablemente QMC5883L (clon)\n", dir, id);
    }
    registrarMagnetometro(dir, "QMC5883L");
}

void identificarHMC5883L(uint8_t dir) {
    uint8_t id[3] = {0, 0, 0};
    if (leerRegistros(dir, 0x0A, id, 3) && id[0] == 'H' && id[1] == '4' && id[2] == '3') {
        Serial.printf("  0x%02X  ID = \"H43\" -> HMC5883L\n", dir);
        registrarMagnetometro(dir, "HMC5883L");
    } else {
        Serial.printf("  0x%02X  responde, pero el ID no es \"H43\"\n", dir);
    }
}

void identificarQMC5883P(uint8_t dir) {
    uint8_t id = 0;
    if (leerRegistros(dir, 0x00, &id, 1) && id == 0x80) {
        // QMC5883P y QMC6308 comparten dirección e ID (misma familia de QST)
        Serial.printf("  0x%02X  ID (reg 0x00) = 0x80 -> QMC5883P o QMC6308\n", dir);
        registrarMagnetometro(dir, "QMC5883P/QMC6308");
    } else {
        Serial.printf("  0x%02X  responde, pero el ID no es 0x80\n", dir);
    }
}

// ── Ciclo completo ───────────────────────────────────────────────────────────
void prepararMPU() {
    for (uint8_t dir = 0x68; dir <= 0x69; dir++) {
        if (!responde(dir)) continue;
        escribirRegistro(dir, MPU_REG_PWR_MGMT_1, 0x00);   // despertar
        delay(10);
        escribirRegistro(dir, MPU_REG_USER_CTRL, 0x00);    // maestro I2C interno apagado
        escribirRegistro(dir, MPU_REG_INT_PIN_CFG, 0x02);  // bypass: el AK8963 queda en el bus
        delay(10);
    }
}

void identificar() {
    dir_acelerometro = dir_magnetometro = 0;
    chip_acelerometro = chip_magnetometro = nullptr;

    Serial.println(F("\n[ID] Activando bypass del MPU (si existe)..."));
    prepararMPU();

    Serial.println(F("[ID] Dispositivos en el bus:"));
    uint8_t encontrados = 0;
    for (uint8_t dir = 0x01; dir < 0x7F; dir++) {
        if (!responde(dir)) continue;
        encontrados++;
        switch (dir) {
            case 0x68: case 0x69: identificarMPU(dir);      break;
            case 0x0C:            identificarAK8963(dir);   break;
            case 0x0D:            identificarQMC5883L(dir); break;
            case 0x1E:            identificarHMC5883L(dir); break;
            case 0x2C:            identificarQMC5883P(dir); break;
            default:
                Serial.printf("  0x%02X  dispositivo no esperado\n", dir);
                break;
        }
    }

    if (encontrados == 0) {
        Serial.println(F("  (ninguno) -> revisar VCC, GND, SDA y SCL; correr 01_escaneo_i2c"));
        return;
    }

    // Resumen para copiar en src/config.h
    Serial.println(F("\n[ID] Para src/config.h:"));
    if (dir_acelerometro) {
        Serial.printf("  #define DIRECCION_ACELEROMETRO  0x%02X   // %s\n",
                      dir_acelerometro, chip_acelerometro);
    } else {
        Serial.println(F("  (no se encontro acelerometro)"));
    }
    if (dir_magnetometro) {
        Serial.printf("  #define DIRECCION_MAGNETOMETRO  0x%02X   // %s\n",
                      dir_magnetometro, chip_magnetometro);
        if (strcmp(chip_magnetometro, "QMC5883L") != 0) {
            Serial.println(F("  AVISO: el driver actual (src/sensores/qmc5883l.cpp) solo sirve para "
                             "QMC5883L; para este chip hay que adaptarlo."));
        }
    } else {
        Serial.println(F("  (no se encontro magnetometro)"));
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    delay(1000);
    Serial.println(F("\n=== ACPI | Prueba 02: identificar sensores ==="));
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
}

void loop() {
    identificar();
    delay(INTERVALO_IDENTIFICAR_MS);
}
