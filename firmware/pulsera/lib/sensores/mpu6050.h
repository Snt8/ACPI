#ifndef MPU6050_H
#define MPU6050_H

//Definimos la clase que controlara el acelerometro
class ControladorAcelerometro {
private:
    //Ejes de la brujula
    static float x;
    static float y;
    static float z;
public:
    static bool inicializar(); //Inicializa el dispositivo
    static void leerDatos(); //Lee los datos del Magnetometro
    static void obtenerPitchRoll(float &pitch, float &roll); //Obtiene los datos de la inclinacion
};


#endif