#ifndef QMC6308_DRIVER_H
#define QMC6308_DRIVER_H

// ── Driver para el magnetómetro QMC6308 (I2C, compatible con pulsera ESP32-C3) ──
// Nota: el archivo conserva el nombre qmc5883l.h por compatibilidad con includes
// existentes, pero la implementación es específica para el QMC6308.
//
// Dirección I2C por defecto: 0x2C
// Secuencia de init:
//   1. Soft reset: escribir 0x01 en reg 0x0D
//   2. Modo continuo: escribir 0x03 en reg 0x0A
// Lectura: 6 bytes desde reg 0x01, orden LSB-first por eje
//
// Calibración hard-iron: Leaky Integrator
//   offset_nuevo = offset_anterior * LEAKY_ALPHA + lectura_actual * LEAKY_BETA
//   valor_corregido = lectura_actual - offset_acumulado

class ControladorMagnetometro {
private:
    // Lecturas crudas corregidas por hard-iron [LSB]
    static float x;
    static float y;
    static float z;

    // Offsets de calibración hard-iron (Leaky Integrator)
    static float offset_x;
    static float offset_y;
    static float offset_z;

public:
    static bool inicializar();
    static bool leerPosicion();  // Retorna false si la lectura I2C falla

    // Calcula el heading compensado por inclinación.
    // Parámetros gx, gy, gz: aceleración normalizada en unidades g (de MPU6050).
    // Fórmula de tilt compensation directa sobre vectores magnéticos.
    static float calcularHeading(float gx, float gy, float gz);
};

#endif // QMC6308_DRIVER_H