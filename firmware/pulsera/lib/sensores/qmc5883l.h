#ifndef QMC5883L_H
#define QMC5883L_H

//Definimos la clase que controlara el magnetometro
class ControladorMagnetometro {
private:
    //Ejes
    static float x;
    static float y;
    static float z;
public:
    static bool inicializar(); //Inicializa el magnetometro
    static void leerPosicion(); //Obtiene la posicion en x, y, z
    static float calcularHeading(const float pitch, const float roll); //Corrige la posicion

};


#endif