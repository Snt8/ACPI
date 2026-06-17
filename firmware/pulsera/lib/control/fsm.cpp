#include "fsm.h"
#include "../motores/motor.h"
#include "../brujula/brujula.h"
#include "../comunicacion/semaforo/semaforo.h"
#include "../comunicacion/movil/movil.h"
#include "../utils/calcular_direccion.h"
#include "../utils/patrones_vibracion.h"
#include "../../include/config.h"
#include "../../include/constantes.h"
#include <Arduino.h>

// ── Definición de variables estáticas ────────────────────────────────────────
EstadoPulsera  MaquinaEstados::estadoActual                = EstadoPulsera::BOOTING;
unsigned long  MaquinaEstados::ultimoTiempoPaqueteSemaforo = 0;
unsigned long  MaquinaEstados::ultimoTiempoEnvioBLE        = 0;

// ── Variables locales del estado SENSING_CROSSING ────────────────────────────
static unsigned long ultimo_muestreo_ms = 0;

// Patron de vibracion activo y motor asignado al patron direccional
enum class PatronActivo { NINGUNO, CONFIRMATORIO, VERDE, DIRECCIONAL };
static PatronActivo            patron_activo    = PatronActivo::NINGUNO;
static ControladorMotores::Motor motor_dir_activo = ControladorMotores::DERECHO;

// ── Helper: cambiar patron y limpiar estado anterior ─────────────────────────
static void activarPatron(PatronActivo nuevo,
                          ControladorMotores::Motor motor = ControladorMotores::AMBOS) {
    bool mismo_patron = (patron_activo == nuevo);
    bool mismo_motor  = (motor_dir_activo == motor);

    if (mismo_patron && (nuevo != PatronActivo::DIRECCIONAL || mismo_motor)) {
        return;  // Sin cambio — no reiniciar
    }

    reiniciarPatrones();  // Apaga motores y resetea estado interno
    patron_activo     = nuevo;
    motor_dir_activo  = motor;
}

// ── Helper: ejecutar el patron activo en cada iteracion del loop ──────────────
static void ejecutarPatron() {
    switch (patron_activo) {
        case PatronActivo::CONFIRMATORIO:
            vibracionConfirmatoria(ControladorMotores::AMBOS);
            break;
        case PatronActivo::VERDE:
            vibracionVerde(ControladorMotores::AMBOS);
            break;
        case PatronActivo::DIRECCIONAL:
            vibracionDireccional(motor_dir_activo);
            break;
        case PatronActivo::NINGUNO:
        default:
            break;
    }
}

// ── Inicialización ────────────────────────────────────────────────────────────
void MaquinaEstados::inicializar() {
    ultimoTiempoPaqueteSemaforo = millis();
    ultimoTiempoEnvioBLE        = millis();
    ultimo_muestreo_ms          = millis();

    setEstado(EstadoPulsera::BOOTING);
}

// ── Registro de paquete ESP-NOW recibido ──────────────────────────────────────
void MaquinaEstados::registrarPaqueteRecibido() {
    ultimoTiempoPaqueteSemaforo = millis();
    if (estadoActual == EstadoPulsera::STANDBY_LOW_POWER) {
        setEstado(EstadoPulsera::SENSING_CROSSING);
    }
}

// ── Getter de estado actual ───────────────────────────────────────────────────
EstadoPulsera MaquinaEstados::obtenerEstadoActual() {
    return estadoActual;
}

// ── Transición de estado ──────────────────────────────────────────────────────
void MaquinaEstados::setEstado(EstadoPulsera nuevoEstado) {
    if (estadoActual == nuevoEstado) return;

    Serial.printf("[FSM] Transicion: %d -> %d\n", (int)estadoActual, (int)nuevoEstado);

    if (estadoActual == EstadoPulsera::SENSING_CROSSING) {
        patron_activo = PatronActivo::NINGUNO;
        reiniciarPatrones();
    }

    estadoActual = nuevoEstado;

    if (estadoActual == EstadoPulsera::STANDBY_LOW_POWER) {
        Serial.println(F("[FSM] Entrando a modo STANDBY (Bajo Consumo)..."));
    }
}

// ── Loop principal ────────────────────────────────────────────────────────────
void MaquinaEstados::actualizar() {
    unsigned long ahora = millis();

    switch (estadoActual) {

        // ── BOOTING ───────────────────────────────────────────────────────────
        case EstadoPulsera::BOOTING:
            setEstado(EstadoPulsera::STANDBY_LOW_POWER);
            break;

        // ── STANDBY_LOW_POWER ─────────────────────────────────────────────────
        case EstadoPulsera::STANDBY_LOW_POWER:
            break;

        // ── SENSING_CROSSING ──────────────────────────────────────────────────
        case EstadoPulsera::SENSING_CROSSING: {

            // Timeout: sin paquetes de semaforo en 30s → volver a standby
            if (ahora - ultimoTiempoPaqueteSemaforo >= 30000UL) {
                setEstado(EstadoPulsera::STANDBY_LOW_POWER);
                break;
            }

            // Ejecutar patron activo en CADA iteracion del loop (no solo cada 500ms).
            // Los tiempos internos de los patrones dependen de llamadas continuas.
            ejecutarPatron();

            // ── Muestreo a 2Hz (cada 500ms) ───────────────────────────────────
            if (ahora - ultimo_muestreo_ms < INTERVALO_MUESTREO_MS) {
                break;
            }
            ultimo_muestreo_ms = ahora;

            // 1. Leer sensores y calcular heading
            ControladorBrujula::obtenerHeading();

            // 2. Validar que la lectura no fue rechazada por el safety check
            if (!ControladorBrujula::headingValido()) {
                activarPatron(PatronActivo::NINGUNO);
                Serial.println(F("[HDG] Lectura invalida — motores apagados"));
                break;
            }

            // 3. Calcular delta para prints de debug
            float hp          = ControladorBrujula::headingActual;
            float hs          = (float)ComunicadorSemaforo::ultimoEstado.grados;
            float delta       = fabsf(hp - hs);
            if (delta > 180.0f) delta = 360.0f - delta;
            float angulo_frente = fabsf(delta - 180.0f);

            Serial.printf("[HDG] Pulsera=%.1f  Semaforo=%d  Delta=%.1f  FrenteA=%.1f  "
                          "Cruce=%s\n",
                          hp, (int)hs, delta, angulo_frente,
                          ComunicadorSemaforo::ultimoEstado.permitidoCruce ? "SI" : "NO");

            // 4. Evaluar orientacion: |delta - 180°| < UMBRAL_CRUCE_GRADOS
            bool orientacionOk = ControladorBrujula::revisarSemaforo(
                                     ComunicadorSemaforo::ultimoEstado.grados);

            // 5. Activar patron segun orientacion y estado del semaforo
            if (orientacionOk) {
                if (ComunicadorSemaforo::ultimoEstado.permitidoCruce) {
                    // permitidoCruce=1: semaforo ROJO para autos → peatón PUEDE cruzar
                    activarPatron(PatronActivo::CONFIRMATORIO);
                    Serial.println(F("[PAT] CONFIRMATORIO — cruce PERMITIDO (verde peaton)"));
                } else {
                    // permitidoCruce=0: semaforo VERDE para autos → peatón DEBE esperar
                    activarPatron(PatronActivo::VERDE);
                    Serial.println(F("[PAT] VERDE — cruce PROHIBIDO (rojo peaton)"));
                }
            } else {
                // Usuario mal orientado → indicar hacia donde debe girar
                float objetivo    = fmod(hs + 180.0f, 360.0f);
                float direccion  = calcularDireccion(objetivo, hp);
                ControladorMotores::Motor motor_destino =
                    (direccion > 0.0f) ? ControladorMotores::DERECHO
                                       : ControladorMotores::IZQUIERDO;

                activarPatron(PatronActivo::DIRECCIONAL, motor_destino);

                Serial.printf("[PAT] DIRECCIONAL %s  (angulo_frente=%.1f > umbral)\n",
                              (motor_destino == ControladorMotores::DERECHO)
                                  ? "DERECHO" : "IZQUIERDO",
                              angulo_frente);
            }

            // 6. Reportar telemetria BLE al movil
            if (ahora - ultimoTiempoEnvioBLE >= 500UL) {
                ultimoTiempoEnvioBLE = ahora;

                uint8_t st;
                if (!orientacionOk) {
                    st = 2;  // Orientacion incorrecta
                } else if (ComunicadorSemaforo::ultimoEstado.permitidoCruce) {
                    st = 1;  // Cruce permitido: orientado, semaforo rojo para autos (verde peaton)
                } else {
                    st = 0;  // Cruce prohibido: orientado pero semaforo verde para autos (rojo peaton)
                }

                ComunicadorMovil::enviarDatos(
                    (uint16_t)ControladorBrujula::headingActual,
                    st,
                    orientacionOk,
                    ComunicadorSemaforo::ultimoEstado.tiempo,
                    -1  // Bateria no disponible
                );
            }
            break;
        }

    }
}
