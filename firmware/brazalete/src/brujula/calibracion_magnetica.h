#ifndef CALIBRACION_MAGNETICA_H
#define CALIBRACION_MAGNETICA_H

#include <stdint.h>

// ── Calibración del magnetómetro (hard-iron) ─────────────────────────────────
// El campo fijo del propio equipo (imanes de los motores, tornillos, la placa)
// se suma a cada lectura y desvía el rumbo. Al girar el equipo en todas
// direcciones las lecturas caen sobre una esfera (un círculo si sólo gira en el
// plano) cuyo centro es ese campo fijo: se ajusta la esfera por mínimos
// cuadrados con TODAS las muestras y su centro se resta de cada lectura.
//
// Se usa el ajuste y no el (máx + mín) / 2: girándolo a mano nunca se llega
// exactamente al extremo de cada eje, y con (máx + mín) / 2 eso dejaba ~2° de
// error, demasiado para el umbral de 5°. El ajuste baja a décimas de grado.
//
// Reemplaza al "leaky integrator" que había antes, que restaba el promedio de
// la propia lectura: con el equipo quieto borraba también el campo de la
// Tierra y el rumbo terminaba siendo ruido, y tras un giro marcaba hacia dónde
// había cambiado el rumbo en vez de hacia dónde se miraba.
//
// El mínimo y el máximo de cada eje se siguen usando para saber si ya se giró
// lo suficiente. Para que un golpe o una lectura mala no los estire ni
// desvíe el ajuste, cada eje usa la mediana de las tres últimas muestras.
//
// No usa Arduino: se prueba en la PC (test/). El brazalete y el semáforo
// tienen una COPIA IDÉNTICA de este archivo y su .cpp: si se cambia uno, hay
// que cambiar el otro.

// Lo que se guarda en la memoria no volátil
struct DatosCalibracion {
    float centro[3];  // campo fijo del equipo, en LSB
    float span[3];    // recorrido (máx − mín) de cada eje al calibrar
};

class CalibracionMagnetica {
public:
    // span_minimo: cuánto tiene que abarcar cada eje (máx − mín, en LSB) para
    // dar la calibración por buena. usa_z = false para un equipo que sólo gira
    // en el plano (el semáforo): se ajusta un círculo en X-Y y Z no se corrige.
    CalibracionMagnetica(float span_minimo, bool usa_z);

    // Olvida todo lo medido
    void reiniciar();

    // Suma una muestra cruda (x, y, z en LSB)
    void agregar(const float crudo[3]);

    // true cuando todos los ejes usados abarcan al menos span_minimo y el
    // ajuste tiene solución
    bool lista() const { return lista_; }

    // Resta el centro. Sin calibración lista devuelve la lectura cruda.
    void corregir(const float crudo[3], float corregido[3]) const;

    // Cuánto abarca un eje hasta ahora (0 si todavía no hay muestras)
    float span(int eje) const;
    float spanMinimo() const { return span_minimo_; }

    // Para guardar y recuperar la calibración (memoria no volátil)
    const DatosCalibracion& datos() const { return datos_; }
    // Carga datos guardados; devuelve false (y no cambia nada) si no forman
    // una calibración lista
    bool cargar(const DatosCalibracion& datos);

private:
    bool spansSuficientes(const float span[3]) const;
    bool resolverAjuste();

    float            span_minimo_;
    bool             usa_z_;
    DatosCalibracion datos_;
    bool             lista_;
    bool             hay_datos_;
    float            minimo_[3];
    float            maximo_[3];
    float            ultimas_[3][3];  // [muestra][eje] para la mediana
    uint8_t          n_ultimas_;
    uint8_t          pos_ultimas_;
    // Ecuaciones normales del ajuste (AᵀA y Aᵀb), en double: con lecturas de
    // ~1000 LSB los términos de cuarto grado no caben en la precisión de float
    double           ata_[4][4];
    double           atb_[4];
};

#endif // CALIBRACION_MAGNETICA_H
