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
// El driver sólo entrega la lectura cruda: la calibración (hard-iron y
// escala) y el rumbo con tilt compensation los calcula ControladorBrujula.

class ControladorMagnetometro {
public:
    static bool inicializar();
    // Lee el campo crudo en LSB (x, y, z). Retorna false si la lectura I2C
    // falla o si el chip avisa que un eje se saturó.
    static bool leerCrudo(float crudo[3]);
    // Deja de medir (modo standby del chip) para ahorrar energía en el standby
    // del brazalete. inicializar() lo vuelve a poner en modo continuo.
    static bool dormir();
};

#endif // QMC5883L_DRIVER_H
