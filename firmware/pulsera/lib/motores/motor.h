#ifndef MOTOR_H
#define MOTOR_H
#include <stdint.h>

//Definimos la clase que controlara los motores
class ControladorMotores {
private:
    //Motor derecho
    static const uint8_t pin_motor_derecho = 10;
    static const uint8_t canal_motor_derecho = 0;
    //Motor izquierdo
    static const uint8_t pin_motor_izquierdo = 2;
    static const uint8_t canal_motor_izquierdo = 1;
    //Configuraciones
    static const uint16_t frecuencia = 1000;
    static const uint8_t resolucion = 8;
public:
    enum Motor { DERECHO, IZQUIERDO, AMBOS};

    //Inicializa ambos motores
    static bool inicializar();
    //Hace vibrar el motor pasado como argumento
    static void vibrarMotor(Motor motor, uint8_t intensidad);
    //Detiene el motor pasado como argumento
    static void detenerMotor(Motor motor);
};
#endif