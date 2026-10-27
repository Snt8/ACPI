#ifndef MOTOR_H
#define MOTOR_H
#include <stdint.h>
#include "../config.h"

// Controlador de los motores vibradores del brazalete.
// Compila con las dos APIs de LEDC del core Arduino-ESP32:
//   v3.x (Arduino IDE / ESP-IDF actuales): ledcAttach(pin, freq, bits) y ledcWrite(pin, duty)
//   v2.x: ledcSetup(canal, ...) + ledcAttachPin(pin, canal) y ledcWrite(canal, duty)
class ControladorMotores {
private:
    // Pines físicos de los motores (definidos en config.h)
    static const uint8_t  pin_motor_derecho   = PIN_MOTOR_DERECHO;
    static const uint8_t  pin_motor_izquierdo = PIN_MOTOR_IZQUIERDO;
    // Canales LEDC: sólo los usa la API v2.x; en v3.x el core los asigna solo
    static const uint8_t  canal_motor_derecho   = 0;
    static const uint8_t  canal_motor_izquierdo = 1;
    // Configuración PWM
    static const uint16_t frecuencia = 1000; // Hz
    static const uint8_t  resolucion = 8;    // bits (0-255)
public:
    enum Motor { DERECHO, IZQUIERDO, AMBOS };

    static bool inicializar();
    static void detenerMotor(Motor motor);
    // Deja cada motor encendido (a su intensidad de constantes.h:
    // INTENSIDAD_MOTOR el derecho, INTENSIDAD_MOTOR_IZQUIERDO el izquierdo) o
    // apagado. Sólo escribe en el PWM cuando el estado cambia. Es la única vía
    // para encender un motor: la FSM lo usa con la salida del ControladorPatrones.
    static void aplicar(bool derecho, bool izquierdo);

private:
    static bool encendido_derecho;
    static bool encendido_izquierdo;
};

#endif
