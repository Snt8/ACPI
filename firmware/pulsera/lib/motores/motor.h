#ifndef MOTOR_H
#define MOTOR_H
#include <stdint.h>

// Controlador de los motores vibradores de la pulsera.
// Usa la API LEDC de Arduino ESP32 v3.x (espressif32@6.9.0):
//   ledcAttach(pin, freq, bits) — reemplaza ledcSetup + ledcAttachPin
//   ledcWrite(pin, duty)        — acepta pin directamente (sin canal explícito)
class ControladorMotores {
private:
    // Pines físicos de los motores
    static const uint8_t  pin_motor_derecho   = 10;
    static const uint8_t  pin_motor_izquierdo = 2;
    // CANALES LEDC (ESP32-C3 requiere canal explícito para ledcSetup/ledcAttachPin)
    // Elegimos canales bajos y fijos; si hay otros usos en el proyecto asegúrese de evitar colisiones
    static const uint8_t  canal_motor_derecho   = 0;
    static const uint8_t  canal_motor_izquierdo = 1;
    // Configuración PWM
    static const uint16_t frecuencia = 1000; // Hz
    static const uint8_t  resolucion = 8;    // bits (0-255)
public:
    enum Motor { DERECHO, IZQUIERDO, AMBOS };

    static bool inicializar();
    static void vibrarMotor(Motor motor, uint8_t intensidad);
    static void detenerMotor(Motor motor);
};

#endif