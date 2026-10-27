#ifndef COMUNICACION_SEMAFORO_H
#define COMUNICACION_SEMAFORO_H

#include <Arduino.h>
#include <stdint.h> //Incluimos para usar enteros de un byte
#include <esp_idf_version.h>
#include <WiFi.h>
#include <esp_now.h>
#include "../../protocolo/protocolo_esp.h" //Incluimos el protocolo de ESP-Now para usar la estructura de la comunicacion

// Receptor ESP-NOW de los paquetes del semáforo.
// El callback de ESP-NOW corre en la tarea de WiFi, no en loop(): por eso sólo
// valida el paquete (tamaño y firma "ACPI") y lo deja en un buffer protegido.
// La FSM lo recoge desde loop() con tomarPaquete(), así toda la lógica corre
// en un único hilo y no hay carreras con el evaluador.
class ComunicadorSemaforo {
public:
    static bool inicializar(); //Enciende el WiFi en modo estación y empieza a escuchar ESP-NOW
    static bool desinicializar(); //Deja de escuchar y apaga el WiFi por completo
    // Copia en 'paquete' el último paquete válido recibido y lo marca como
    // leído. Devuelve false si no llegó ninguno desde la última llamada.
    static bool tomarPaquete(estado_semaforo& paquete);
    static bool hayPaquete(); //true si hay un paquete sin tomar
    static void descartarPendiente(); //Olvida el paquete sin tomar, si lo hay
    static uint32_t paquetesDescartados(); //Paquetes ESP-NOW rechazados por tamaño o firma
private:
    // La firma del callback cambió en ESP-IDF 5 (core Arduino-ESP32 v3.x): ahora recibe
    // esp_now_recv_info_t en vez de la MAC remota. Se compila la que corresponda.
#if ESP_IDF_VERSION_MAJOR >= 5
    static void onDatosRecibidos(const esp_now_recv_info_t *info, const uint8_t *data, int data_len); //Callback que convierte los datos a la estructura
#else
    static void onDatosRecibidos(const uint8_t *mac_addr, const uint8_t *data, int data_len); //Callback que convierte los datos a la estructura
#endif

};

#endif
