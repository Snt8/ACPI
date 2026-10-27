#ifndef MPU6050_H
#define MPU6050_H

// ── Controlador del acelerómetro MPU6050 (I2C 0x68) ──────────────────────────
// Los valores x, y, z internos están normalizados en unidades g tras la lectura.
class ControladorAcelerometro {
private:
    static float x;  // Aceleración normalizada eje X [g]
    static float y;  // Aceleración normalizada eje Y [g]
    static float z;  // Aceleración normalizada eje Z [g]

public:
    static bool inicializar();
    static bool leerDatos();  // Retorna false si la lectura I2C falla

    // Devuelve los valores normalizados en g para tilt compensation directa
    static float getGx() { return x; }
    static float getGy() { return y; }
    static float getGz() { return z; }

    // Obtiene pitch y roll en grados (mantiene compatibilidad con código existente)
    static void obtenerPitchRoll(float &pitch, float &roll);
};

#endif // MPU6050_H