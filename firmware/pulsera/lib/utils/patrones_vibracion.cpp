#include <Arduino.h>
#include "patrones_vibracion.h"
#include "../motores/motor.h"
#include "../../include/constantes.h"

// ── Patrón ROJO (cruce autorizado) ───────────────────────────────────────────
// 3 pulsos largos con pausa amplia entre ellos, luego silencio, repite.
// Sensación: lenta y deliberada.
// Estados: 0=encender, 1=ON, 2=pausa entre pulsos, 3=silencio final

static uint8_t       fase_rojo  = 0;
static unsigned long t_rojo     = 0;
static uint8_t       pulso_rojo = 0;  // 0-2

void vibracionConfirmatoria(ControladorMotores::Motor motor) {
    unsigned long ahora = millis();

    switch (fase_rojo) {
        case 0:
            ControladorMotores::vibrarMotor(motor, INTENSIDAD_MOTOR);
            t_rojo    = ahora;
            fase_rojo = 1;
            break;

        case 1:
            if (ahora - t_rojo >= DURACION_PULSO_ROJO_MS) {
                ControladorMotores::detenerMotor(motor);
                t_rojo = ahora;
                pulso_rojo++;
                fase_rojo = (pulso_rojo < 3) ? 2 : 3;
            }
            break;

        case 2:  // pausa entre pulsos
            if (ahora - t_rojo >= PAUSA_ROJO_MS) {
                fase_rojo = 0;
            }
            break;

        case 3:  // silencio final antes de repetir
            if (ahora - t_rojo >= SILENCIO_ROJO_MS) {
                fase_rojo  = 0;
                pulso_rojo = 0;
            }
            break;

        default:
            fase_rojo  = 0;
            pulso_rojo = 0;
            break;
    }
}

// ── Patrón VERDE (semaforo en verde, esperar) ─────────────────────────────────
// 3 pulsos rápidos espaciados a 500 ms, luego 3 s de silencio, repite.
// Sensación: ritmo rápido y repetitivo.

static uint8_t       fase_verde  = 0;
static unsigned long t_verde     = 0;
static uint8_t       pulso_verde = 0;  // 0-2

void vibracionVerde(ControladorMotores::Motor motor) {
    unsigned long ahora = millis();

    switch (fase_verde) {
        case 0:
            ControladorMotores::vibrarMotor(motor, INTENSIDAD_MOTOR);
            t_verde    = ahora;
            fase_verde = 1;
            break;

        case 1:
            if (ahora - t_verde >= DURACION_PULSO_VERDE_MS) {
                ControladorMotores::detenerMotor(motor);
                t_verde = ahora;
                pulso_verde++;
                fase_verde = (pulso_verde < 3) ? 2 : 3;
            }
            break;

        case 2:  // pausa entre pulsos
            if (ahora - t_verde >= PAUSA_VERDE_MS) {
                fase_verde = 0;
            }
            break;

        case 3:  // silencio largo antes de repetir
            if (ahora - t_verde >= SILENCIO_VERDE_MS) {
                fase_verde  = 0;
                pulso_verde = 0;
            }
            break;

        default:
            fase_verde  = 0;
            pulso_verde = 0;
            break;
    }
}

// ── Patrón DIRECCIONAL ────────────────────────────────────────────────────────
// Un pulso en el motor izquierdo o derecho cada ~900 ms.

static uint8_t                   fase_dir         = 0;
static unsigned long             t_dir            = 0;
static ControladorMotores::Motor ultimo_motor_dir = ControladorMotores::DERECHO;
static bool                      dir_iniciado     = false;

void vibracionDireccional(ControladorMotores::Motor motor) {
    unsigned long ahora = millis();

    if (dir_iniciado && motor != ultimo_motor_dir) {
        ControladorMotores::detenerMotor(ultimo_motor_dir);
        fase_dir     = 0;
        dir_iniciado = false;
    }
    ultimo_motor_dir = motor;

    switch (fase_dir) {
        case 0:
            ControladorMotores::vibrarMotor(motor, INTENSIDAD_MOTOR);
            t_dir        = ahora;
            fase_dir     = 1;
            dir_iniciado = true;
            break;

        case 1:
            if (ahora - t_dir >= DURACION_PULSO_DIR_MS) {
                ControladorMotores::detenerMotor(motor);
                t_dir    = ahora;
                fase_dir = 2;
            }
            break;

        case 2:
            if (ahora - t_dir >= PAUSA_DIR_MS) {
                fase_dir = 0;
            }
            break;

        default:
            fase_dir = 0;
            break;
    }
}

// ── Reinicio de estado ────────────────────────────────────────────────────────
void reiniciarPatrones() {
    fase_rojo   = 0;  pulso_rojo  = 0;  t_rojo  = 0;
    fase_verde  = 0;  pulso_verde = 0;  t_verde = 0;
    fase_dir    = 0;  dir_iniciado = false;        t_dir   = 0;
    ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
}
