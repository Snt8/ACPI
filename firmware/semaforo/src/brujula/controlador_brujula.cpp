#include "controlador_brujula.h"
#include <Wire.h>
#include <Arduino.h>
#include <math.h>
#include <Preferences.h>
#include "calibracion_magnetica.h"
#include "../const.h"
#include "../config.h"

// ── Mapa de registros QMC6308 ─────────────────────────────────────────────────
#define QMC6308_ADDR       0x2C   // Dirección I2C por defecto
#define QMC6308_REG_DATA   0x01   // Primer registro de datos (6 bytes: Xl,Xh,Yl,Yh,Zl,Zh)
#define QMC6308_REG_CTRL2  0x0A   // Registro de control 2 (modo de operación)
#define QMC6308_REG_CTRL3  0x0D   // Registro de control 3 (soft reset)

#define QMC6308_MODE_CONT  0x03   // Modo continuo, 200Hz
#define QMC6308_SOFT_RESET 0x01   // Soft reset

// ── Variables estáticas ───────────────────────────────────────────────────────
float ControladorBrujulaSemaforo::headingActual = 0.0f;

namespace {

// Sólo X e Y: el semáforo gira en el plano
CalibracionMagnetica calibracion(CALIBRACION_SPAN_MINIMO_LSB, false);
bool calibracion_cargada = false;

const char* const NVS_ESPACIO = "brujula";
const char* const NVS_CLAVE   = "cal_esfera";

// Promedio circular: se promedian los vectores (cos, sin), no los grados,
// para que 359° y 1° den 0° y no 180°
float lecturas_cos[LECTURAS_PROMEDIO];
float lecturas_sin[LECTURAS_PROMEDIO];
uint8_t n_lecturas   = 0;
uint8_t pos_lecturas = 0;

void cargarCalibracion() {
    Preferences prefs;
    DatosCalibracion d;
    bool ok = prefs.begin(NVS_ESPACIO, true);
    if (ok) {
        ok = prefs.getBytesLength(NVS_CLAVE) == sizeof(d) &&
             prefs.getBytes(NVS_CLAVE, &d, sizeof(d)) == sizeof(d) &&
             calibracion.cargar(d);
        prefs.end();
    }
    if (ok) {
        Serial.printf("[SEMAFORO QMC6308] Calibracion cargada: centro (%.0f, %.0f) LSB\n",
                      d.centro[0], d.centro[1]);
    } else {
        Serial.println(F("[SEMAFORO QMC6308] Sin calibracion guardada"));
    }
}

bool guardarCalibracion() {
    Preferences prefs;
    if (!prefs.begin(NVS_ESPACIO, false)) return false;
    bool ok = prefs.putBytes(NVS_CLAVE, &calibracion.datos(), sizeof(DatosCalibracion)) ==
              sizeof(DatosCalibracion);
    prefs.end();
    return ok;
}

// Lee el campo crudo (LSB) del QMC6308
bool leerCrudo(float crudo[3]) {
    // Apuntar al registro de datos
    Wire.beginTransmission(QMC6308_ADDR);
    Wire.write(QMC6308_REG_DATA);
    if (Wire.endTransmission(false) != 0) {  // repeated start
        return false;
    }

    // Solicitar 6 bytes
    Wire.requestFrom((uint8_t)QMC6308_ADDR, (uint8_t)6);
    if (Wire.available() < 6) {
        return false;
    }

    // QMC6308 entrega los datos LSB primero (Little-Endian por eje)
    // Orden: Xl, Xh, Yl, Yh, Zl, Zh
    // Se leen primero a un arreglo: en "Wire.read() | (Wire.read() << 8)" C++ no
    // garantiza cuál de las dos lecturas se hace primero, y el LSB y el MSB
    // podían quedar intercambiados (el brazalete ya lo corrigió así)
    uint8_t datos[6];
    for (uint8_t i = 0; i < 6; i++) {
        datos[i] = Wire.read();
    }
    crudo[0] = (float)(int16_t)(datos[0] | (datos[1] << 8));
    crudo[1] = (float)(int16_t)(datos[2] | (datos[3] << 8));
    crudo[2] = (float)(int16_t)(datos[4] | (datos[5] << 8));
    return true;
}

} // namespace

// ── Escáner I2C de diagnóstico ────────────────────────────────────────────────
// Se ejecuta UNA vez al boot para reportar la dirección real del sensor.
static void escanearI2C_semaforo() {
    Serial.println(F("[SEMAFORO I2C SCAN] Buscando dispositivos..."));
    uint8_t encontrados = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[SEMAFORO I2C SCAN] Dispositivo en 0x%02X\n", addr);
            encontrados++;
        }
    }
    if (encontrados == 0) {
        Serial.println(F("[SEMAFORO I2C SCAN] No se encontraron dispositivos."));
    }
}

// ── Inicialización ────────────────────────────────────────────────────────────
bool ControladorBrujulaSemaforo::inicializar() {
    // El bus I2C ya lo inició setup() con los pines de config.h; un Wire.begin()
    // sin pines aquí lo volvería a configurar con los pines por defecto

    // Paso 0: escanear bus I2C para confirmar dirección física real del QMC6308
    escanearI2C_semaforo();

    // Paso 1: Soft reset (escribe 0x01 en reg 0x0D)
    Wire.beginTransmission(QMC6308_ADDR);
    Wire.write(QMC6308_REG_CTRL3);
    Wire.write(QMC6308_SOFT_RESET);
    if (Wire.endTransmission() != 0) {
        Serial.println(F("[SEMAFORO QMC6308] ERROR: No responde en 0x2C (soft reset)"));
        return false;
    }
    delay(10);  // Esperar que el reset se complete

    // Paso 2: Configurar modo continuo (escribe 0x03 en reg 0x0A)
    Wire.beginTransmission(QMC6308_ADDR);
    Wire.write(QMC6308_REG_CTRL2);
    Wire.write(QMC6308_MODE_CONT);
    if (Wire.endTransmission() != 0) {
        Serial.println(F("[SEMAFORO QMC6308] ERROR: Fallo al configurar modo continuo"));
        return false;
    }

    Serial.println(F("[SEMAFORO QMC6308] Inicializado correctamente en 0x2C"));
    if (!calibracion_cargada) {
        calibracion_cargada = true;
        cargarCalibracion();
    }
    return true;
}

// ── Lectura con calibración y promedio ───────────────────────────────────────
bool ControladorBrujulaSemaforo::leerHeading() {
    float crudo[3], m[3];
    if (!leerCrudo(crudo)) return false;
    calibracion.corregir(crudo, m);  // sin calibración, la lectura cruda

    // ── Heading directo (sin tilt compensation — semáforo fijo y horizontal) ──
    // El factor de escala se cancela en atan2(y, x), no es necesario dividir.
    float angulo = atan2f(m[1], m[0]);
    lecturas_cos[pos_lecturas] = cosf(angulo);
    lecturas_sin[pos_lecturas] = sinf(angulo);
    pos_lecturas = (uint8_t)((pos_lecturas + 1) % LECTURAS_PROMEDIO);
    if (n_lecturas < LECTURAS_PROMEDIO) n_lecturas++;

    float c = 0.0f, s = 0.0f;
    for (uint8_t i = 0; i < n_lecturas; i++) {
        c += lecturas_cos[i];
        s += lecturas_sin[i];
    }
    float promedio = atan2f(s, c) * RAD_A_GRADOS;

    // Normalizar al rango [0, 360)
    if (promedio < 0.0f) {
        promedio += GRADOS_CIRCUNFERENCIA;
    }
    headingActual = (promedio >= GRADOS_CIRCUNFERENCIA) ? 0.0f : promedio;
    return true;
}

uint16_t ControladorBrujulaSemaforo::gradosParaEnviar() {
    // Redondeo (antes se truncaba: −0.5° de sesgo); 359.6° → 360 → 0
    return (uint16_t)(lroundf(headingActual) % 360);
}

// ── Calibración ──────────────────────────────────────────────────────────────
bool ControladorBrujulaSemaforo::calibrada() {
    return calibracion.lista();
}

void ControladorBrujulaSemaforo::borrarCalibracion() {
    Preferences prefs;
    if (prefs.begin(NVS_ESPACIO, false)) {
        prefs.remove(NVS_CLAVE);
        prefs.end();
    }
    calibracion.reiniciar();
}

bool ControladorBrujulaSemaforo::calibrar(unsigned long duracion_max_ms) {
    Serial.println(F("[CALIBRACION] Gira el semaforo, plano, al menos una vuelta completa y despacio,"));
    Serial.println(F("[CALIBRACION] lejos de objetos metalicos (antes de fijarlo al poste)."));
    calibracion.reiniciar();

    unsigned long inicio = millis();
    unsigned long ultimo_crecimiento = inicio;
    unsigned long ultimo_aviso = inicio;
    float spans_ref[2] = {0, 0};

    while (millis() - inicio < duracion_max_ms) {
        float crudo[3];
        if (leerCrudo(crudo)) calibracion.agregar(crudo);
        unsigned long ahora = millis();

        // ¿X o Y crecieron más de un 1 % desde la última vez que crecieron?
        bool crecio = false;
        for (int i = 0; i < 2; i++) {
            if (calibracion.span(i) > spans_ref[i] * 1.01f) {
                spans_ref[i] = calibracion.span(i);
                crecio = true;
            }
        }
        if (crecio) ultimo_crecimiento = ahora;

        if (ahora - ultimo_aviso >= 1000) {
            ultimo_aviso = ahora;
            Serial.printf("[CALIBRACION] Recorrido X %4.0f  Y %4.0f  (minimo %.0f c/u)%s\n",
                          calibracion.span(0), calibracion.span(1), calibracion.spanMinimo(),
                          calibracion.lista() ? "  LISTA, sigue girando" : "");
        }

        if (calibracion.lista() && ahora - ultimo_crecimiento >= CALIBRACION_ESTABLE_MS) {
            bool guardada = guardarCalibracion();
            const DatosCalibracion& d = calibracion.datos();
            Serial.printf("[CALIBRACION] Lista: centro (%.0f, %.0f) LSB%s\n",
                          d.centro[0], d.centro[1],
                          guardada ? ", guardada" : " — [ERROR] no se pudo guardar");
            n_lecturas = 0;  // el promedio arranca de nuevo con la calibración
            return true;
        }
        delay(CALIBRACION_INTERVALO_MS);
    }

    Serial.println(F("[CALIBRACION] No se completo: se envia el rumbo SIN calibrar."));
    Serial.println(F("[CALIBRACION] Envia 'c' por el monitor serie para intentarlo de nuevo."));
    calibracion.reiniciar();
    return false;
}
