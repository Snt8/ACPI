#ifndef EJES_ACELEROMETRO_H
#define EJES_ACELEROMETRO_H

// ── Orientación del acelerómetro respecto al magnetómetro ─────────────────────
// La tilt compensation necesita la gravedad en los MISMOS ejes que el campo
// magnético. En la PCB v3.5 el módulo MPU está rotado 270° y el GY-271 90°, así
// que los ejes de los dos chips pueden no coincidir, y con los ejes cruzados el
// rumbo cambia decenas de grados al inclinar la muñeca (29° a 20° de
// inclinación con un giro de 180°).
//
// Se detecta sola durante la calibración: el ángulo entre la gravedad y el
// campo de la Tierra (la inclinación magnética, ~27° bajo el horizonte en
// Colombia) es el mismo en cualquier pose, pero sólo si los ejes están bien
// alineados. Se prueban las 8 orientaciones posibles de un módulo montado
// plano sobre la placa (girado 0/90/180/270°, normal o boca abajo) y se elige
// la que deja ese ángulo más constante.
//
// No usa Arduino: se prueba en la PC (test/).

const int NUM_ORIENTACIONES_ACEL = 8;

// Lleva la lectura del acelerómetro (en su lugar) a los ejes del magnetómetro
void aplicarOrientacionAcel(int orientacion, float g[3]);

// "0", "90", "180", "270", "volteado 0", ...
const char* nombreOrientacionAcel(int orientacion);

// Elige la orientación con la que el ángulo entre g y m varía menos.
//   g[i]: acelerómetro (en g, ejes propios); m[i]: campo YA calibrado.
//   Sólo usa las muestras con |g| entre 0.9 y 1.1 g (muñeca sin acelerar).
//   dispersion (opcional): desviación estándar, en grados, del ángulo para
//   cada orientación.
// Devuelve -1 si hay menos de 30 muestras utilizables.
int detectarOrientacionAcel(const float (*g)[3], const float (*m)[3], int n,
                            float dispersion[NUM_ORIENTACIONES_ACEL]);

#endif // EJES_ACELEROMETRO_H
