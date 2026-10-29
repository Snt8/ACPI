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

float rumboCompensado(float mx, float my, float mz, float gx, float gy, float gz) {
    //Con g hacia arriba, el este es m × g y el norte es la parte horizontal de m:
    //  norte = m − (m·ĝ)ĝ        este = m × g
    //El rumbo del eje x es atan2(este_x, norte_x). Proyectar m al plano
    //horizontal y tomar atan2(y, x) en el marco del sensor (la fórmula de antes)
    //sólo es exacto con la muñeca plana: daba 4° de error a 30° de inclinación
    //y 16° a 30°/30°.
    float g2 = gx * gx + gy * gy + gz * gz;
    float m_dot_g = mx * gx + my * gy + mz * gz;
    float norte_x = mx - (m_dot_g / g2) * gx;
    float este_x  = my * gz - mz * gy;   // el módulo de g se cancela en atan2
    float g_norma = sqrtf(g2);
    float rumbo = atan2f(este_x / g_norma, norte_x) * RAD_A_GRADOS;
    if (rumbo < 0.0f) {
        rumbo += GRADOS_CIRCUNFERENCIA;
    }
    //atan2 de un ángulo diminuto negativo + 360 puede redondear a 360 exacto
    return (rumbo >= GRADOS_CIRCUNFERENCIA) ? 0.0f : rumbo;
}
