#include "calibracion_magnetica.h"
#include <math.h>

namespace {

float mediana3(float a, float b, float c) {
    if (a > b) { float t = a; a = b; b = t; }
    if (b > c) { b = c; }
    return (a > b) ? a : b;
}

// Resuelve el sistema n×n (n ≤ 4) por eliminación gaussiana con pivoteo.
// false si es singular (las muestras no alcanzan para el ajuste).
bool resolver(double a[4][4], double b[4], int n, double x[4]) {
    for (int col = 0; col < n; col++) {
        int piv = col;
        for (int f = col + 1; f < n; f++) {
            if (fabs(a[f][col]) > fabs(a[piv][col])) piv = f;
        }
        if (fabs(a[piv][col]) < 1e-9) return false;
        if (piv != col) {
            for (int k = 0; k < n; k++) { double t = a[col][k]; a[col][k] = a[piv][k]; a[piv][k] = t; }
            double t = b[col]; b[col] = b[piv]; b[piv] = t;
        }
        for (int f = col + 1; f < n; f++) {
            double factor = a[f][col] / a[col][col];
            for (int k = col; k < n; k++) a[f][k] -= factor * a[col][k];
            b[f] -= factor * b[col];
        }
    }
    for (int f = n - 1; f >= 0; f--) {
        double s = b[f];
        for (int k = f + 1; k < n; k++) s -= a[f][k] * x[k];
        x[f] = s / a[f][f];
    }
    return true;
}

} // namespace

CalibracionMagnetica::CalibracionMagnetica(float span_minimo, bool usa_z)
    : span_minimo_(span_minimo), usa_z_(usa_z) {
    reiniciar();
}

void CalibracionMagnetica::reiniciar() {
    for (int i = 0; i < 3; i++) {
        datos_.centro[i] = 0.0f;
        datos_.span[i]   = 0.0f;
        minimo_[i] = maximo_[i] = 0.0f;
    }
    for (int i = 0; i < 4; i++) {
        atb_[i] = 0.0;
        for (int k = 0; k < 4; k++) ata_[i][k] = 0.0;
    }
    lista_       = false;
    hay_datos_   = false;
    n_ultimas_   = 0;
    pos_ultimas_ = 0;
}

void CalibracionMagnetica::agregar(const float crudo[3]) {
    for (int i = 0; i < 3; i++) ultimas_[pos_ultimas_][i] = crudo[i];
    pos_ultimas_ = (uint8_t)((pos_ultimas_ + 1) % 3);
    if (n_ultimas_ < 3) n_ultimas_++;
    if (n_ultimas_ < 3) return;  // hace falta la mediana de tres

    float m[3];
    for (int i = 0; i < 3; i++) {
        m[i] = mediana3(ultimas_[0][i], ultimas_[1][i], ultimas_[2][i]);
        if (!hay_datos_ || m[i] < minimo_[i]) minimo_[i] = m[i];
        if (!hay_datos_ || m[i] > maximo_[i]) maximo_[i] = m[i];
    }
    hay_datos_ = true;

    // Esfera: x² + y² + z² = 2cx·x + 2cy·y + 2cz·z + k  →  fila [x, y, z, 1]
    // Círculo (sin Z): x² + y² = 2cx·x + 2cy·y + k     →  fila [x, y, 1]
    double fila[4];
    int n = 0;
    fila[n++] = m[0];
    fila[n++] = m[1];
    if (usa_z_) fila[n++] = m[2];
    fila[n++] = 1.0;
    double b = (double)m[0] * m[0] + (double)m[1] * m[1];
    if (usa_z_) b += (double)m[2] * m[2];
    for (int i = 0; i < n; i++) {
        atb_[i] += fila[i] * b;
        for (int k = 0; k < n; k++) ata_[i][k] += fila[i] * fila[k];
    }

    for (int i = 0; i < 3; i++) datos_.span[i] = maximo_[i] - minimo_[i];
    lista_ = spansSuficientes(datos_.span) && resolverAjuste();
}

bool CalibracionMagnetica::resolverAjuste() {
    int n = usa_z_ ? 4 : 3;
    double a[4][4], b[4], x[4];
    for (int i = 0; i < n; i++) {
        b[i] = atb_[i];
        for (int k = 0; k < n; k++) a[i][k] = ata_[i][k];
    }
    if (!resolver(a, b, n, x)) return false;
    datos_.centro[0] = (float)(x[0] / 2.0);
    datos_.centro[1] = (float)(x[1] / 2.0);
    datos_.centro[2] = usa_z_ ? (float)(x[2] / 2.0) : 0.0f;
    return true;
}

float CalibracionMagnetica::span(int eje) const {
    return hay_datos_ ? maximo_[eje] - minimo_[eje] : 0.0f;
}

bool CalibracionMagnetica::spansSuficientes(const float span[3]) const {
    int ejes = usa_z_ ? 3 : 2;
    for (int i = 0; i < ejes; i++) {
        if (!(span[i] >= span_minimo_)) return false;  // también descarta NaN
    }
    return true;
}

void CalibracionMagnetica::corregir(const float crudo[3], float corregido[3]) const {
    for (int i = 0; i < 3; i++) {
        corregido[i] = lista_ ? crudo[i] - datos_.centro[i] : crudo[i];
    }
}

bool CalibracionMagnetica::cargar(const DatosCalibracion& datos) {
    for (int i = 0; i < 3; i++) {
        if (!isfinite(datos.centro[i])) return false;
    }
    if (!spansSuficientes(datos.span)) return false;
    reiniciar();
    datos_ = datos;
    lista_ = true;
    return true;
}
