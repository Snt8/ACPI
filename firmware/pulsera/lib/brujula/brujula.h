#ifndef BRUJULA_H
#define BRUJULA_H
//Incluimos bibliotecas necesarias
#include <stdint.h>

//Definimos la clase que gestionara la brujula (magnetometro y acelerometro)
class ControladorBrujula {
private:
    static float headingActual;
public:
    static bool inicializar(); //Inicializa ambos sensores
    static void obtenerHeading(); //Lee los sensores y retorna el heading compensado
    static bool revisarSemaforo(uint16_t posicionSemaforo); //Compara en base al heading para determinar si es el semaforo correcto
};

#endif