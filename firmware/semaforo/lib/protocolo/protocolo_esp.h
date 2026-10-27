#ifndef PROTOCOLO_ESP_H
#define PROTOCOLO_ESP_H
// Contrato del paquete ESP-NOW que el semáforo envía al brazalete.
// Este archivo es una COPIA IDÉNTICA en firmware/semaforo/lib/protocolo/ y
// firmware/brazalete/src/protocolo/: si se cambia uno, hay que cambiar el otro.
//Importamos stdint para usar ints de menor tamaño
#include <stdint.h>
#include <string.h>

// Firma fija al inicio de cada paquete. ESP-NOW recibe cualquier broadcast del
// canal; sin la firma, un paquete ajeno del mismo tamaño reiniciaría el
// temporizador del modo standby del brazalete como si fuera del semáforo.
#define FIRMA_SEMAFORO_LARGO 4
static const char FIRMA_SEMAFORO[FIRMA_SEMAFORO_LARGO] = {'A', 'C', 'P', 'I'};

//Definimos la estructura con la que se envian y reciben los datos
//Del semaforo al brazalete. #pragma pack(1) evita bytes de relleno entre campos
//(funciona en GCC y en MSVC, que compila los tests del brazalete en la PC)
#pragma pack(push, 1)
typedef struct {
    char firma[FIRMA_SEMAFORO_LARGO]; //Siempre "ACPI" (sin terminador '\0')
    uint8_t id; //Identificador del semaforo en la red local
    uint16_t grados; //Contiene el heading del cruce (grados)
    uint8_t permitidoCruce; //1 = el peaton puede cruzar (rojo para autos), 0 = debe esperar
    uint8_t tiempo; //Tiempo restante para que el semaforo cambie de estado (s)
} estado_semaforo;
#pragma pack(pop)

static_assert(sizeof(estado_semaforo) == 9, "estado_semaforo debe medir 9 bytes en ambos extremos");

// Rellena la firma de un paquete antes de enviarlo (lo usa el semáforo)
static inline void firmarPaqueteSemaforo(estado_semaforo* paquete) {
    memcpy(paquete->firma, FIRMA_SEMAFORO, FIRMA_SEMAFORO_LARGO);
}

// true si los bytes recibidos son un paquete del semáforo ACPI: tamaño exacto
// y firma correcta (lo usa el brazalete al recibir)
static inline bool esPaqueteSemaforo(const uint8_t* datos, int largo) {
    return datos != nullptr
        && largo == (int)sizeof(estado_semaforo)
        && memcmp(datos, FIRMA_SEMAFORO, FIRMA_SEMAFORO_LARGO) == 0;
}

#endif // PROTOCOLO_ESP_H
