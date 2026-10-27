// ── Prueba 03: lecturas de los sensores y funcionamiento de los motores ──────
// Sketch independiente del firmware (no usa src/), para validar el hardware:
//   - Detecta solo el acelerómetro (MPU6050 / MPU6500 / MPU9250) y el
//     magnetómetro (QMC5883L, HMC5883L, QMC5883P/QMC6308 o AK8963 del MPU9250).
//   - Imprime acelerómetro, giroscopio, temperatura y magnetómetro a 2 Hz.
//   - Al arrancar hace vibrar primero el motor DERECHO y luego el IZQUIERDO,
//     para comprobar que no quedaron invertidos.
//   - Acepta comandos por el monitor serie (escribir la letra y Enter):
//       d = pulso motor derecho     i = pulso motor izquierdo   a = ambos
//       b = barrido de intensidad (muestra desde qué duty arranca cada motor)
//       + / - = subir / bajar intensidad   l = pausar / reanudar lecturas
//       D / I = dejar fijo el derecho / izquierdo a la intensidad actual (para medir con
//               el multímetro; la misma letra lo apaga)   0 = apagar ambos
//       r = volver a detectar sensores     h = ayuda
//
// Arduino IDE: placa "ESP32C3 Dev Module", "USB CDC On Boot: Enabled",
// monitor serie a 115200 baudios con "Nueva línea" o "Sin ajuste de línea".

#include <Wire.h>
#include <math.h>

#if ESP_ARDUINO_VERSION_MAJOR < 3
#error "Este sketch usa la API LEDC del core Arduino-ESP32 3.x: actualizar 'esp32 by Espressif' en el Gestor de placas"
#endif

#define BAUD_RATE             115200
#define PIN_I2C_SDA           8
#define PIN_I2C_SCL           9
#define PIN_MOTOR_DERECHO     10
#define PIN_MOTOR_IZQUIERDO   3

#define FRECUENCIA_PWM        1000   // Hz
#define RESOLUCION_PWM        8      // bits (0-255)
#define INTENSIDAD_INICIAL    120    // mismo valor que INTENSIDAD_MOTOR del firmware (180 era demasiado)
#define PASO_INTENSIDAD       25
#define DURACION_PULSO_MS     1000UL
#define INTERVALO_LECTURA_MS  500UL  // 2 Hz, igual que el firmware

#define RAD_A_GRADOS          (180.0f / (float)M_PI)

// Va antes de cualquier función: Arduino IDE inserta los prototipos automáticos
// arriba de la primera función, y nombreMagnetometro() recibe este tipo
enum TipoMagnetometro { MAG_NINGUNO, MAG_QMC5883L, MAG_HMC5883L, MAG_QMC5883P, MAG_AK8963 };

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

int16_t msbPrimero(const uint8_t* b) { return (int16_t)((b[0] << 8) | b[1]); }
int16_t lsbPrimero(const uint8_t* b) { return (int16_t)(b[0] | (b[1] << 8)); }

// ── Acelerómetro / giroscopio ────────────────────────────────────────────────
uint8_t dir_mpu = 0;
bool    mpu_es_6050 = true;  // cambia la fórmula de temperatura

bool detectarMPU() {
    dir_mpu = 0;
    for (uint8_t dir = 0x68; dir <= 0x69; dir++) {
        if (!responde(dir)) continue;

        uint8_t id = 0;
        leerRegistros(dir, 0x75, &id, 1);       // WHO_AM_I
        escribirRegistro(dir, 0x6B, 0x00);      // PWR_MGMT_1: despertar
        delay(10);
        escribirRegistro(dir, 0x1B, 0x00);      // GYRO_CONFIG: ±250 °/s
        escribirRegistro(dir, 0x1C, 0x00);      // ACCEL_CONFIG: ±2 g
        escribirRegistro(dir, 0x6A, 0x00);      // USER_CTRL: maestro I2C interno apagado
        escribirRegistro(dir, 0x37, 0x02);      // INT_PIN_CFG: bypass (deja ver el AK8963)
        delay(10);

        dir_mpu     = dir;
        mpu_es_6050 = (id == 0x68);
        Serial.printf("[MPU] 0x%02X, WHO_AM_I = 0x%02X\n", dir, id);
        return true;
    }
    Serial.println(F("[MPU] No se encontro en 0x68 ni 0x69"));
    return false;
}

void leerMPU() {
    uint8_t d[14];  // ACCEL X,Y,Z | TEMP | GYRO X,Y,Z (MSB primero)
    if (!leerRegistros(dir_mpu, 0x3B, d, 14)) {
        Serial.println(F("[MPU] ERROR de lectura"));
        return;
    }
    float ax = msbPrimero(&d[0])  / 16384.0f;  // g en ±2 g
    float ay = msbPrimero(&d[2])  / 16384.0f;
    float az = msbPrimero(&d[4])  / 16384.0f;
    int16_t t_raw = msbPrimero(&d[6]);
    float gx = msbPrimero(&d[8])  / 131.0f;    // °/s en ±250 °/s
    float gy = msbPrimero(&d[10]) / 131.0f;
    float gz = msbPrimero(&d[12]) / 131.0f;
    float temp = mpu_es_6050 ? (t_raw / 340.0f + 36.53f) : (t_raw / 333.87f + 21.0f);

    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_A_GRADOS;
    float roll  = atan2f(ay, az) * RAD_A_GRADOS;
    float modulo = sqrtf(ax * ax + ay * ay + az * az);  // en reposo debe dar ~1.00 g

    Serial.printf("[ACC] x=%+.2f y=%+.2f z=%+.2f g |g|=%.2f  pitch=%+6.1f roll=%+6.1f\n",
                  ax, ay, az, modulo, pitch, roll);
    Serial.printf("[GIR] x=%+7.1f y=%+7.1f z=%+7.1f deg/s  T=%.1f C\n", gx, gy, gz, temp);
}

// ── Magnetómetro ─────────────────────────────────────────────────────────────
TipoMagnetometro tipo_mag = MAG_NINGUNO;
uint8_t          dir_mag  = 0;

const char* nombreMagnetometro(TipoMagnetometro tipo) {
    switch (tipo) {
        case MAG_QMC5883L: return "QMC5883L";
        case MAG_HMC5883L: return "HMC5883L";
        case MAG_QMC5883P: return "QMC5883P/QMC6308";
        case MAG_AK8963:   return "AK8963 (MPU9250)";
        default:           return "ninguno";
    }
}

// Se prueba primero el GY-271 y sólo al final el AK8963 interno del MPU9250
bool detectarMagnetometro() {
    tipo_mag = MAG_NINGUNO;
    uint8_t id[3] = {0, 0, 0};

    if (responde(0x0D)) {
        // QMC5883L: periodo SET/RESET recomendado y modo continuo 200 Hz, ±8 G, OSR 512
        escribirRegistro(0x0D, 0x0A, 0x80);  // soft reset
        delay(10);
        escribirRegistro(0x0D, 0x0B, 0x01);
        escribirRegistro(0x0D, 0x09, 0x1D);
        tipo_mag = MAG_QMC5883L; dir_mag = 0x0D;
    } else if (responde(0x1E) && leerRegistros(0x1E, 0x0A, id, 3) && id[0] == 'H') {
        // HMC5883L: 8 muestras promediadas a 15 Hz, ±1.3 Ga, modo continuo
        escribirRegistro(0x1E, 0x00, 0x70);
        escribirRegistro(0x1E, 0x01, 0x20);
        escribirRegistro(0x1E, 0x02, 0x00);
        tipo_mag = MAG_HMC5883L; dir_mag = 0x1E;
    } else if (responde(0x2C) && leerRegistros(0x2C, 0x00, id, 1) && id[0] == 0x80) {
        // QMC5883P / QMC6308: signo de ejes, SET/RESET con ±8 G, modo normal 200 Hz
        escribirRegistro(0x2C, 0x29, 0x06);
        escribirRegistro(0x2C, 0x0B, 0x08);
        escribirRegistro(0x2C, 0x0A, 0xCD);
        tipo_mag = MAG_QMC5883P; dir_mag = 0x2C;
    } else if (responde(0x0C) && leerRegistros(0x0C, 0x00, id, 1) && id[0] == 0x48) {
        // AK8963: salida de 16 bits, medición continua a 100 Hz
        escribirRegistro(0x0C, 0x0A, 0x00);  // power-down antes de cambiar de modo
        delay(10);
        escribirRegistro(0x0C, 0x0A, 0x16);
        tipo_mag = MAG_AK8963; dir_mag = 0x0C;
    }

    if (tipo_mag == MAG_NINGUNO) {
        Serial.println(F("[MAG] No se encontro magnetometro"));
        return false;
    }
    delay(20);  // primera medición disponible
    Serial.printf("[MAG] 0x%02X, %s\n", dir_mag, nombreMagnetometro(tipo_mag));
    return true;
}

void leerMagnetometro() {
    uint8_t d[7];
    int16_t mx, my, mz;
    bool ok = false;

    switch (tipo_mag) {
        case MAG_QMC5883L:  // X, Y, Z desde 0x00, LSB primero
            ok = leerRegistros(dir_mag, 0x00, d, 6);
            mx = lsbPrimero(&d[0]); my = lsbPrimero(&d[2]); mz = lsbPrimero(&d[4]);
            break;
        case MAG_HMC5883L:  // ojo: el orden es X, Z, Y y MSB primero
            ok = leerRegistros(dir_mag, 0x03, d, 6);
            mx = msbPrimero(&d[0]); mz = msbPrimero(&d[2]); my = msbPrimero(&d[4]);
            break;
        case MAG_QMC5883P:  // X, Y, Z desde 0x01, LSB primero
            ok = leerRegistros(dir_mag, 0x01, d, 6);
            mx = lsbPrimero(&d[0]); my = lsbPrimero(&d[2]); mz = lsbPrimero(&d[4]);
            break;
        case MAG_AK8963:    // X, Y, Z desde 0x03, LSB primero; leer ST2 libera la siguiente
            ok = leerRegistros(dir_mag, 0x03, d, 7);
            mx = lsbPrimero(&d[0]); my = lsbPrimero(&d[2]); mz = lsbPrimero(&d[4]);
            break;
        default:
            return;
    }

    if (!ok) {
        Serial.println(F("[MAG] ERROR de lectura"));
        return;
    }

    // Rumbo sin calibrar ni compensar inclinación: sólo para ver que cambia al girar
    float rumbo = atan2f((float)my, (float)mx) * RAD_A_GRADOS;
    if (rumbo < 0.0f) rumbo += 360.0f;

    Serial.printf("[MAG] x=%6d y=%6d z=%6d  rumbo plano=%5.1f deg\n", mx, my, mz, rumbo);
}

// ── Motores ──────────────────────────────────────────────────────────────────
uint8_t intensidad = INTENSIDAD_INICIAL;

void pulsoMotor(uint8_t pin, const char* nombre) {
    Serial.printf("[MOTOR] %s (GPIO%d) a %u/255 durante %lu ms\n",
                  nombre, pin, intensidad, DURACION_PULSO_MS);
    ledcWrite(pin, intensidad);
    delay(DURACION_PULSO_MS);
    ledcWrite(pin, 0);
}

void pulsoAmbos() {
    Serial.printf("[MOTOR] AMBOS a %u/255 durante %lu ms\n", intensidad, DURACION_PULSO_MS);
    ledcWrite(PIN_MOTOR_DERECHO, intensidad);
    ledcWrite(PIN_MOTOR_IZQUIERDO, intensidad);
    delay(DURACION_PULSO_MS);
    ledcWrite(PIN_MOTOR_DERECHO, 0);
    ledcWrite(PIN_MOTOR_IZQUIERDO, 0);
}

// Sube el duty de a poco: anotar desde qué valor se siente la vibración
void barridoMotor(uint8_t pin, const char* nombre) {
    Serial.printf("[BARRIDO] %s (GPIO%d)\n", nombre, pin);
    for (int duty = 0; duty <= 255; duty += 15) {
        Serial.printf("  duty = %3d\n", duty);
        ledcWrite(pin, duty);
        delay(400);
    }
    ledcWrite(pin, 0);
}

// Encendido fijo a la intensidad actual (no al 100 %: arriesga las soldaduras)
bool fijo_derecho = false, fijo_izquierdo = false;

void alternarFijo(uint8_t pin, const char* nombre, bool& encendido) {
    encendido = !encendido;
    ledcWrite(pin, encendido ? intensidad : 0);
    Serial.printf("[MOTOR] %s (GPIO%d) fijo a %u/255: %s\n",
                  nombre, pin, intensidad, encendido ? "ENCENDIDO" : "apagado");
}

void apagarMotores() {
    fijo_derecho = fijo_izquierdo = false;
    ledcWrite(PIN_MOTOR_DERECHO, 0);
    ledcWrite(PIN_MOTOR_IZQUIERDO, 0);
    Serial.println(F("[MOTOR] Ambos apagados"));
}

// ── Comandos por el monitor serie ────────────────────────────────────────────
bool lecturas_activas = true;

void mostrarAyuda() {
    Serial.println(F("\nComandos: d=derecho  i=izquierdo  a=ambos  b=barrido  "
                     "+/-=intensidad  D/I=fijo  0=apagar  "
                     "l=pausar lecturas  r=redetectar  h=ayuda"));
    Serial.printf("Intensidad actual: %u/255\n\n", intensidad);
}

void atenderComando(char c) {
    switch (c) {
        case 'd': pulsoMotor(PIN_MOTOR_DERECHO, "DERECHO"); break;
        case 'i': pulsoMotor(PIN_MOTOR_IZQUIERDO, "IZQUIERDO"); break;
        case 'a': pulsoAmbos(); break;
        case 'D': alternarFijo(PIN_MOTOR_DERECHO, "DERECHO", fijo_derecho); break;
        case 'I': alternarFijo(PIN_MOTOR_IZQUIERDO, "IZQUIERDO", fijo_izquierdo); break;
        case '0': apagarMotores(); break;
        case 'b':
            barridoMotor(PIN_MOTOR_DERECHO, "DERECHO");
            barridoMotor(PIN_MOTOR_IZQUIERDO, "IZQUIERDO");
            break;
        case '+':
            intensidad = (intensidad > 255 - PASO_INTENSIDAD) ? 255 : intensidad + PASO_INTENSIDAD;
            Serial.printf("[MOTOR] Intensidad = %u/255\n", intensidad);
            break;
        case '-':
            intensidad = (intensidad < PASO_INTENSIDAD) ? 0 : intensidad - PASO_INTENSIDAD;
            Serial.printf("[MOTOR] Intensidad = %u/255\n", intensidad);
            break;
        case 'l':
            lecturas_activas = !lecturas_activas;
            Serial.println(lecturas_activas ? F("[LECTURAS] Reanudadas") : F("[LECTURAS] En pausa"));
            break;
        case 'r':
            detectarMPU();
            detectarMagnetometro();
            break;
        case 'h': case '?':
            mostrarAyuda();
            break;
        default:
            break;  // Enter, espacios, etc.
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    delay(1000);
    Serial.println(F("\n=== ACPI | Prueba 03: sensores y motores ==="));

    // Motores apagados desde el primer momento
    ledcAttach(PIN_MOTOR_DERECHO, FRECUENCIA_PWM, RESOLUCION_PWM);
    ledcAttach(PIN_MOTOR_IZQUIERDO, FRECUENCIA_PWM, RESOLUCION_PWM);
    ledcWrite(PIN_MOTOR_DERECHO, 0);
    ledcWrite(PIN_MOTOR_IZQUIERDO, 0);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    detectarMPU();
    detectarMagnetometro();

    Serial.println(F("\n[MOTOR] Secuencia inicial: debe vibrar el lado DERECHO y luego el IZQUIERDO"));
    pulsoMotor(PIN_MOTOR_DERECHO, "DERECHO");
    delay(500);
    pulsoMotor(PIN_MOTOR_IZQUIERDO, "IZQUIERDO");

    mostrarAyuda();
}

void loop() {
    while (Serial.available()) {
        atenderComando((char)Serial.read());
    }

    static unsigned long ultima_lectura = 0;
    if (lecturas_activas && millis() - ultima_lectura >= INTERVALO_LECTURA_MS) {
        ultima_lectura = millis();
        if (dir_mpu) leerMPU();
        if (tipo_mag != MAG_NINGUNO) leerMagnetometro();
        Serial.println();
    }
}
