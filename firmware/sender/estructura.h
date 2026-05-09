#ifndef ESTRUCTURA_H
#define ESTRUCTURA_H

// Contiene el estado del semáforo y su angulo.
typedef struct esp_now_data_structure {
  int estado; // 1 para VERDE/ENCENDIDO, 2 para ROJO/APAGADO
  int angulo; // Ángulo de orientación del semáforo (ej: 0, 90, 180, 270)
} esp_now_data_structure;

#endif
