#include "ejes_acelerometro.h"
#include <math.h>
#include "../constantes.h"

void aplicarOrientacionAcel(int orientacion, float g[3]) {
    float x = g[0], y = g[1], z = g[2];
    switch (orientacion) {
        case 1: g[0] = -y; g[1] =  x; g[2] =  z; break;  // girado 90°
        case 2: g[0] = -x; g[1] = -y; g[2] =  z; break;  // girado 180°
        case 3: g[0] =  y; g[1] = -x; g[2] =  z; break;  // girado 270°
        case 4: g[0] =  x; g[1] = -y; g[2] = -z; break;  // boca abajo
        case 5: g[0] =  y; g[1] =  x; g[2] = -z; break;  // boca abajo + 90°
        case 6: g[0] = -x; g[1] =  y; g[2] = -z; break;  // boca abajo + 180°
        case 7: g[0] = -y; g[1] = -x; g[2] = -z; break;  // boca abajo + 270°
        default: break;                                  // alineado
    }
}

const char* nombreOrientacionAcel(int orientacion) {
    static const char* const NOMBRES[NUM_ORIENTACIONES_ACEL] = {
        "0", "90", "180", "270", "volteado 0", "volteado 90", "volteado 180", "volteado 270"};
    return (orientacion >= 0 && orientacion < NUM_ORIENTACIONES_ACEL) ? NOMBRES[orientacion] : "?";
}

int detectarOrientacionAcel(const float (*g)[3], const float (*m)[3], int n,
                            float dispersion[NUM_ORIENTACIONES_ACEL]) {
    int mejor = -1;
    float mejor_disp = 1e9f;
    for (int o = 0; o < NUM_ORIENTACIONES_ACEL; o++) {
        double suma = 0.0, suma2 = 0.0;
        int usadas = 0;
        for (int i = 0; i < n; i++) {
            float gg[3] = {g[i][0], g[i][1], g[i][2]};
            float ng = sqrtf(gg[0] * gg[0] + gg[1] * gg[1] + gg[2] * gg[2]);
            float nm = sqrtf(m[i][0] * m[i][0] + m[i][1] * m[i][1] + m[i][2] * m[i][2]);
            if (ng < 0.9f || ng > 1.1f || nm < 1.0f) continue;
            aplicarOrientacionAcel(o, gg);
            float c = (gg[0] * m[i][0] + gg[1] * m[i][1] + gg[2] * m[i][2]) / (ng * nm);
            if (c > 1.0f) c = 1.0f;
            if (c < -1.0f) c = -1.0f;
            double angulo = acos((double)c) * RAD_A_GRADOS;
            suma  += angulo;
            suma2 += angulo * angulo;
            usadas++;
        }
        if (usadas < 30) return -1;
        double media = suma / usadas;
        double var = suma2 / usadas - media * media;
        float disp = (float)sqrt(var > 0.0 ? var : 0.0);
        if (dispersion != nullptr) dispersion[o] = disp;
        if (disp < mejor_disp) {
            mejor_disp = disp;
            mejor = o;
        }
    }
    return mejor;
}

namespace {
// Sólo las muestras con la muñeca sin acelerar, como en detectarOrientacionAcel
bool quieta(const float g[3], float& norma) {
    norma = sqrtf(g[0] * g[0] + g[1] * g[1] + g[2] * g[2]);
    return norma >= 0.9f && norma <= 1.1f;
}
}  // namespace

float fraccionInclinadaAcel(const float (*g)[3], int n, float grados) {
    double media[3] = {0.0, 0.0, 0.0};
    int usadas = 0;
    for (int i = 0; i < n; i++) {
        float ng;
        if (!quieta(g[i], ng)) continue;
        for (int k = 0; k < 3; k++) media[k] += g[i][k] / ng;
        usadas++;
    }
    if (usadas == 0) return 0.0f;
    double nm = sqrt(media[0] * media[0] + media[1] * media[1] + media[2] * media[2]);
    if (nm < 1e-6) return 1.0f;  // poses opuestas que se anulan: muy variadas
    const double umbral = cos(grados / RAD_A_GRADOS);
    int inclinadas = 0;
    for (int i = 0; i < n; i++) {
        float ng;
        if (!quieta(g[i], ng)) continue;
        double c = (g[i][0] * media[0] + g[i][1] * media[1] + g[i][2] * media[2]) / (ng * nm);
        if (c < umbral) inclinadas++;
    }
    return (float)inclinadas / (float)usadas;
}

bool orientacionAcelClara(const float (*g)[3], int n, int o,
                          const float dispersion[NUM_ORIENTACIONES_ACEL]) {
    if (o < 0 || o >= NUM_ORIENTACIONES_ACEL) return false;
    if (fraccionInclinadaAcel(g, n, INCLINACION_MINIMA_EJES_GRADOS) < FRACCION_INCLINADA_MINIMA) return false;
    for (int k = 0; k < NUM_ORIENTACIONES_ACEL; k++) {
        if (k != o && !(dispersion[k] >= MARGEN_SEGUNDA_EJES * dispersion[o])) return false;
    }
    return true;
}
