#ifndef QMC5883L_DRIVER_H
#define QMC5883L_DRIVER_H

// ── Driver para el magnetómetro QMC5883L (módulo GY-271, I2C) ─────────────────
// Dirección I2C fija: 0x0D (chip ID 0xFF en reg 0x0D)
// Secuencia de init:
//   1. Soft reset: escribir 0x80 en reg 0x0A (Control 2)
//   2. Periodo SET/RESET: escribir 0x01 en reg 0x0B (recomendado por el datasheet)
//   3. Modo continuo, 200 Hz, ±8 G, OSR 512: escribir 0x1D en reg 0x09 (Control 1)
// Lectura: 6 bytes desde reg 0x00, orden LSB-first por eje (X, Y, Z)
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

#endif // QMC5883L_DRIVER_H
