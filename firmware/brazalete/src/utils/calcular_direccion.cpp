//Incluimos la cabecera
#include "calcular_direccion.h"
//Sólo matemáticas estándar: así este archivo también compila en los tests de la PC
#include <math.h>
//Incluimos las constantes de configuracion
#include "../constantes.h"

float calcularDireccion(float posicionSemaforo, float headingActual) {
    //Calculamos la diferencia angular positiva usando aritmetica modular
    float diferencia = fmodf(posicionSemaforo - headingActual + GRADOS_CIRCUNFERENCIA, GRADOS_CIRCUNFERENCIA);
    //fmodf conserva el signo: si la resta quedó por debajo de -360 el resultado es negativo
    if (diferencia < 0.0f) {
        diferencia += GRADOS_CIRCUNFERENCIA;
    }
    //En caso de que el giro sea mayor a 180 grados, resulta mas rapido dar el giro al contrario
    if (diferencia > GRADOS_CIRCUNFERENCIA / 2) {
        diferencia -= GRADOS_CIRCUNFERENCIA; //Convertimos el angulo en su equivalente negativo
    }
    return diferencia;
}

float arcoMenor(float a, float b) {
    //Diferencia angular absoluta, llevada al arco menor [0, 180]
    float diff = fabsf(fmodf(a - b, GRADOS_CIRCUNFERENCIA));
    return (diff > GRADOS_CIRCUNFERENCIA / 2) ? (GRADOS_CIRCUNFERENCIA - diff) : diff;
}

float giroParaEnfrentar(float headingBrazalete, float headingSemaforo) {
    //De frente al semáforo = mirando hacia el lado opuesto a su heading
    float objetivo = fmodf(headingSemaforo + GRADOS_CIRCUNFERENCIA / 2, GRADOS_CIRCUNFERENCIA);
    return calcularDireccion(objetivo, headingBrazalete);
}

float desviacionEnfrentamiento(float headingBrazalete, float headingSemaforo) {
    //Los nodos se ENFRENTAN cuando Δθ ≈ 180°: la desviación es cuánto falta para eso
    float delta_theta = arcoMenor(headingBrazalete, headingSemaforo);
    return fabsf(delta_theta - GRADOS_CIRCUNFERENCIA / 2);
}
