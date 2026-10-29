#include "brujula.h"
#include "calibracion_magnetica.h"
#include "ejes_acelerometro.h"
#include "../sensores/mpu6050.h"
#include "../sensores/qmc5883l.h"
#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include "../config.h"
#include "../constantes.h"
#include "../utils/calcular_direccion.h"

static_assert(ROTACION_ACELEROMETRO_GRADOS == 0 || ROTACION_ACELEROMETRO_GRADOS == 90 ||
              ROTACION_ACELEROMETRO_GRADOS == 180 || ROTACION_ACELEROMETRO_GRADOS == 270,
              "ROTACION_ACELEROMETRO_GRADOS debe ser 0, 90, 180 o 270");

// ── Variables estáticas ───────────────────────────────────────────────────────
float        ControladorBrujula::headingActual      = 0.0f;
float        ControladorBrujula::headingAnterior    = 0.0f;
unsigned long ControladorBrujula::tiempoUltimaLectura = 0;
bool          ControladorBrujula::lecturaValida     = false;

namespace {

CalibracionMagnetica calibracion(CALIBRACION_SPAN_MINIMO_LSB, true);
bool calibracion_cargada = false;  // ya se intentó leer la guardada

// Memoria no volátil (NVS): espacio y clave de la calibración. La versión
// cambia si cambia el formato de DatosCalibracion.
const char* const NVS_ESPACIO = "brujula";
const char* const NVS_CLAVE   = "cal_v2";  // v2: con detección de ejes (obliga a recalibrar)
const char* const NVS_CLAVE_EJES = "ejes_acel";

// Orientación del acelerómetro respecto al magnetómetro (ejes_acelerometro.h).
// Se detecta al calibrar y se guarda; mientras no haya una guardada se usa la
// de ROTACION_ACELEROMETRO_GRADOS (config.h).
int orientacion_acel = ROTACION_ACELEROMETRO_GRADOS / 90;

// Muestras (acelerómetro, magnetómetro crudo) que se guardan durante la
// calibración para detectar la orientación del acelerómetro. Si se llena, se
// queda con una de cada dos y sigue guardando la mitad de seguido.
const int MAX_MUESTRAS_EJES = 400;
float muestras_g[MAX_MUESTRAS_EJES][3];
float muestras_m[MAX_MUESTRAS_EJES][3];
int   n_muestras_ejes = 0;
int   paso_muestras = 1;
int   contador_muestras = 0;

void guardarMuestraEjes(const float g[3], const float m[3]) {
    if (++contador_muestras < paso_muestras) return;
    contador_muestras = 0;
    if (n_muestras_ejes == MAX_MUESTRAS_EJES) {
        for (int i = 0; i < MAX_MUESTRAS_EJES / 2; i++) {
            for (int k = 0; k < 3; k++) {
                muestras_g[i][k] = muestras_g[2 * i][k];
                muestras_m[i][k] = muestras_m[2 * i][k];
            }
        }
        n_muestras_ejes = MAX_MUESTRAS_EJES / 2;
        paso_muestras *= 2;
    }
    for (int k = 0; k < 3; k++) {
        muestras_g[n_muestras_ejes][k] = g[k];
        muestras_m[n_muestras_ejes][k] = m[k];
    }
    n_muestras_ejes++;
}

// Con la calibración ya lista, elige la orientación del acelerómetro y la guarda
void detectarEjes(const CalibracionMagnetica& cal) {
    for (int i = 0; i < n_muestras_ejes; i++) cal.corregir(muestras_m[i], muestras_m[i]);
    float disp[NUM_ORIENTACIONES_ACEL];
    int o = detectarOrientacionAcel(muestras_g, muestras_m, n_muestras_ejes, disp);
    if (o < 0) {
        Serial.println(F("[CALIBRACION] No hubo muestras quietas para revisar los ejes del acelerometro"));
        return;
    }
    // La segunda mejor, para ver si la elección es clara
    float segunda = 1e9f;
    for (int k = 0; k < NUM_ORIENTACIONES_ACEL; k++) {
        if (k != o && disp[k] < segunda) segunda = disp[k];
    }
    Serial.printf("[CALIBRACION] Acelerometro girado %s respecto al magnetometro "
                  "(variacion %.1f deg; la siguiente opcion %.1f deg)\n",
                  nombreOrientacionAcel(o), disp[o], segunda);
    if (segunda < 2.0f * disp[o]) {
        Serial.println(F("[CALIBRACION] AVISO: la eleccion no es clara; calibra de nuevo inclinando mas"));
    }
    if (o != ROTACION_ACELEROMETRO_GRADOS / 90) {
        Serial.printf("[CALIBRACION] (config.h decia %d; se usa lo detectado)\n", ROTACION_ACELEROMETRO_GRADOS);
    }
    orientacion_acel = o;
    Preferences prefs;
    if (prefs.begin(NVS_ESPACIO, false)) {
        prefs.putUChar(NVS_CLAVE_EJES, (uint8_t)o);
        prefs.end();
    }
}

void cargarCalibracion() {
    Preferences prefs;
    if (!prefs.begin(NVS_ESPACIO, true)) {
        Serial.println(F("[BRUJULA] Sin calibracion guardada"));
        return;
    }
    DatosCalibracion d;
    bool ok = prefs.getBytesLength(NVS_CLAVE) == sizeof(d) &&
              prefs.getBytes(NVS_CLAVE, &d, sizeof(d)) == sizeof(d) &&
              calibracion.cargar(d);
    uint8_t ejes = prefs.getUChar(NVS_CLAVE_EJES, 0xFF);
    if (ejes < NUM_ORIENTACIONES_ACEL) orientacion_acel = ejes;
    prefs.end();
    if (ok) {
        Serial.printf("[BRUJULA] Acelerometro girado %s respecto al magnetometro\n",
                      nombreOrientacionAcel(orientacion_acel));
        Serial.printf("[BRUJULA] Calibracion cargada: centro (%.0f, %.0f, %.0f) LSB\n",
                      d.centro[0], d.centro[1], d.centro[2]);
    } else {
        Serial.println(F("[BRUJULA] Sin calibracion guardada"));
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

// Promedia MUESTRAS_POR_LECTURA muestras de cada sensor. false si alguna falla.
bool leerPromedio(float mag[3], float g[3]) {
    for (int i = 0; i < 3; i++) mag[i] = g[i] = 0.0f;
    for (int n = 0; n < MUESTRAS_POR_LECTURA; n++) {
        float m[3];
        if (!ControladorAcelerometro::leerDatos() || !ControladorMagnetometro::leerCrudo(m)) {
            return false;
        }
        g[0] += ControladorAcelerometro::getGx();
        g[1] += ControladorAcelerometro::getGy();
        g[2] += ControladorAcelerometro::getGz();
        for (int i = 0; i < 3; i++) mag[i] += m[i];
        if (n + 1 < MUESTRAS_POR_LECTURA) delay(PAUSA_ENTRE_MUESTRAS_MS);
    }
    for (int i = 0; i < 3; i++) {
        mag[i] /= MUESTRAS_POR_LECTURA;
        g[i]   /= MUESTRAS_POR_LECTURA;
    }
    aplicarOrientacionAcel(orientacion_acel, g);
    return true;
}

} // namespace

// ── Inicialización ────────────────────────────────────────────────────────────
bool ControladorBrujula::inicializar() {
    bool acelerometro_ok = ControladorAcelerometro::inicializar();
    bool magnetometro_ok = ControladorMagnetometro::inicializar();

    if (!calibracion_cargada) {
        calibracion_cargada = true;
        cargarCalibracion();
    }

    if (acelerometro_ok && magnetometro_ok) {
        tiempoUltimaLectura = millis();
        return true;
    }
    return false;
}

// ── Bajo consumo (standby) ───────────────────────────────────────────────────
void ControladorBrujula::dormir() {
    ControladorAcelerometro::dormir();
    ControladorMagnetometro::dormir();
}

bool ControladorBrujula::despertar() {
    // inicializar() vuelve a configurar los dos chips y reinicia la referencia
    // del safety check, así la primera lectura tras el standby no se compara
    // con una de hace más de 30 s
    lecturaValida = false;
    return inicializar();
}

// ── Lectura de sensores con calibración, tilt compensation y safety check ────
void ControladorBrujula::obtenerHeading() {
    // 1. Promedio de varias muestras de los dos sensores
    float mag[3], g[3];
    bool sensores_ok = leerPromedio(mag, g);
    unsigned long ahora = millis();

    // Si cualquier sensor falló, invalidar lectura inmediatamente — no continuar
    if (!sensores_ok) {
        lecturaValida = false;
        tiempoUltimaLectura = ahora;  // Resetear referencia para evitar bypass del safety check
        return;
    }

    // 2. Sin calibración el rumbo puede estar desviado decenas de grados
    if (!calibracion.lista()) {
        lecturaValida = false;
        Serial.println(F("[BRUJULA] Sin calibrar: no se indica orientacion"));
        return;
    }

    // 3. La gravedad sólo sirve si la muñeca está quieta (mide ~1 g)
    float g_norma = sqrtf(g[0] * g[0] + g[1] * g[1] + g[2] * g[2]);
    if (fabsf(g_norma - 1.0f) > TOLERANCIA_GRAVEDAD_G) {
        lecturaValida = false;
        tiempoUltimaLectura = ahora;
        Serial.printf("[BRUJULA] Muneca en movimiento (%.2f g) — lectura invalida\n", g_norma);
        return;
    }

    // 4. Calibración y rumbo con tilt compensation
    float m[3];
    calibracion.corregir(mag, m);
    float nuevo_heading = rumboCompensado(m[0], m[1], m[2], g[0], g[1], g[2]);
    nuevo_heading = fmodf(nuevo_heading + CORRECCION_MONTAJE_GRADOS + GRADOS_CIRCUNFERENCIA,
                          GRADOS_CIRCUNFERENCIA);

    // ── Safety check ──────────────────────────────────────────────────────────
    // Si el heading cambia más de UMBRAL_DELTA_HEADING grados respecto a la
    // lectura anterior y ésta tiene menos de UMBRAL_TIEMPO_SAFETY_MS (dos
    // muestras seguidas), se considera una lectura inválida (movimiento brusco
    // o ruido). No se actualiza headingActual.
    unsigned long dt = ahora - tiempoUltimaLectura;

    if (dt < UMBRAL_TIEMPO_SAFETY_MS) {
        // Diferencia angular mínima en el círculo
        float delta = arcoMenor(nuevo_heading, headingAnterior);

        if (delta > UMBRAL_DELTA_HEADING) {
            // Lectura rechazada: cambio de heading demasiado rápido
            lecturaValida = false;
            Serial.printf("[BRUJULA] Safety: delta=%.1f deg en %lums — lectura invalida\n",
                          delta, dt);
            tiempoUltimaLectura = ahora;
            headingAnterior = nuevo_heading;  // actualizar referencia para el próximo ciclo
            return;
        }
    }

    // Lectura válida: actualizar estado
    headingActual       = nuevo_heading;
    headingAnterior     = nuevo_heading;
    tiempoUltimaLectura = ahora;
    lecturaValida       = true;
}

// ── Estado de validez de la última lectura ────────────────────────────────────
bool ControladorBrujula::headingValido() {
    return lecturaValida;
}

// ── Calibración ──────────────────────────────────────────────────────────────
bool ControladorBrujula::calibrada() {
    return calibracion.lista();
}

void ControladorBrujula::borrarCalibracion() {
    Preferences prefs;
    if (prefs.begin(NVS_ESPACIO, false)) {
        prefs.remove(NVS_CLAVE);
        prefs.remove(NVS_CLAVE_EJES);
        prefs.end();
    }
    calibracion.reiniciar();
    lecturaValida = false;
}

bool ControladorBrujula::calibrar() {
    Serial.println(F("[CALIBRACION] Gira el brazalete en todas direcciones (tambien boca abajo),"));
    Serial.println(F("[CALIBRACION] como dibujando un 8, lejos de objetos metalicos y del celular."));
    calibracion.reiniciar();
    n_muestras_ejes = 0;
    paso_muestras = 1;
    contador_muestras = 0;

    unsigned long inicio = millis();
    unsigned long ultimo_crecimiento = inicio;
    unsigned long ultimo_aviso = inicio;
    float spans_ref[3] = {0, 0, 0};

    while (millis() - inicio < CALIBRACION_DURACION_MAX_MS) {
        float m[3];
        if (ControladorMagnetometro::leerCrudo(m)) {
            calibracion.agregar(m);
            if (ControladorAcelerometro::leerDatos()) {
                float g[3] = {ControladorAcelerometro::getGx(), ControladorAcelerometro::getGy(),
                              ControladorAcelerometro::getGz()};
                guardarMuestraEjes(g, m);
            }
        }
        unsigned long ahora = millis();

        // ¿Algún eje creció más de un 1 % desde la última vez que creció?
        bool crecio = false;
        for (int i = 0; i < 3; i++) {
            if (calibracion.span(i) > spans_ref[i] * 1.01f) {
                spans_ref[i] = calibracion.span(i);
                crecio = true;
            }
        }
        if (crecio) ultimo_crecimiento = ahora;

        if (ahora - ultimo_aviso >= 1000) {
            ultimo_aviso = ahora;
            Serial.printf("[CALIBRACION] Recorrido X %4.0f  Y %4.0f  Z %4.0f  (minimo %.0f c/u)%s\n",
                          calibracion.span(0), calibracion.span(1), calibracion.span(2),
                          calibracion.spanMinimo(), calibracion.lista() ? "  LISTA, sigue girando" : "");
        }

        if (calibracion.lista() && ahora - ultimo_crecimiento >= CALIBRACION_ESTABLE_MS) {
            bool guardada = guardarCalibracion();
            const DatosCalibracion& d = calibracion.datos();
            Serial.printf("[CALIBRACION] Lista: centro (%.0f, %.0f, %.0f) LSB%s\n",
                          d.centro[0], d.centro[1], d.centro[2],
                          guardada ? ", guardada" : " — [ERROR] no se pudo guardar");
            detectarEjes(calibracion);
            lecturaValida = false;
            return true;
        }
        delay(CALIBRACION_INTERVALO_MS);
    }

    Serial.println(F("[CALIBRACION] [ERROR] No se completo: el brazalete no indicara orientacion."));
    Serial.println(F("[CALIBRACION] Envia 'c' por el monitor serie para intentarlo de nuevo."));
    calibracion.reiniciar();
    return false;
}
