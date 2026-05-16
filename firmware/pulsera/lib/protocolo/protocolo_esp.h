#ifndef PROTOCOLO_ESP_H
#define PROTOCOLO_ESP_H
//Importamos stdint para usar ints de menor tamaño
#include <stdint.h>

//Definimos la estructura con la que se envian y reciben los datos
//Del semaforo a la pulsera
typedef struct __attribute__((packed)){
    uint8_t id; //Identificador de la  direccion MAC del dispositivo
    uint16_t grados; //Contiene el heading del cruce (grados)
    uint8_t permitidoCruce; //Si esta en rojo o en verde (0, 1)
    uint8_t tiempo; //Tiempo restante para que el semaforo cambie de estado
} estado_semaforo;

#endif // PROTOCOLO_ESP_H
