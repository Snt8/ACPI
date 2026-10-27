#ifndef BRAZALETE_CALCULAR_DIRECCION_H
#define BRAZALETE_CALCULAR_DIRECCION_H

//Definimos la funcion para determinar la orientacion del semaforo.
//Devuelve el giro más corto en grados (-180, 180] para llevar headingActual a
//posicionSemaforo: positivo = girar a la derecha, negativo = a la izquierda.
float calcularDireccion(float posicionSemaforo, float headingActual);

//Arco menor entre dos headings, en grados [0, 180]. Ej.: 350° y 10° -> 20°.
float arcoMenor(float a, float b);

//Giro más corto para quedar ENFRENTADO al semáforo, es decir, mirando hacia
//headingSemaforo + 180°. Mismo signo que calcularDireccion; su valor absoluto
//es desviacionEnfrentamiento().
float giroParaEnfrentar(float headingBrazalete, float headingSemaforo);

//Cuántos grados le faltan al brazalete para quedar ENFRENTADO al semáforo,
//es decir |Δθ − 180°| con Δθ = arco menor entre ambos headings. Va de 0
//(de frente) a 180 (mirando hacia el mismo lado que el semáforo).
float desviacionEnfrentamiento(float headingBrazalete, float headingSemaforo);

#endif
