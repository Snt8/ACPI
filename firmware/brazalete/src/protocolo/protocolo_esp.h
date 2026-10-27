#ifndef PROTOCOLO_ESP_H
#define PROTOCOLO_ESP_H
// Contrato del paquete ESP-NOW que el semáforo envía al brazalete.
// Este archivo es una COPIA IDÉNTICA en firmware/semaforo/src/protocolo/ y
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
    uint16_t grados; //Contiene el heading del cruce (grados, 0-359)
    uint8_t permitidoCruce; //1 = el peaton puede cruzar (rojo para autos), 0 = debe esperar
    uint8_t tiempo; //Tiempo restante para que el semaforo cambie de estado (s)
    uint8_t verificacion; //CRC-8 de todos los bytes anteriores (ver crc8Semaforo)
} estado_semaforo;
#pragma pack(pop)

static_assert(sizeof(estado_semaforo) == 10, "estado_semaforo debe medir 10 bytes en ambos extremos");

// CRC-8 (polinomio 0x07, valor inicial 0x00). Con interferencia de radio
// (celulares, computadores, audífonos en 2.4 GHz) lo normal es que el paquete
// se pierda; esta verificación descarta además cualquier paquete que llegue
// con bytes alterados o que otro equipo arme con la misma firma por casualidad.
static inline uint8_t crc8Semaforo(const uint8_t* datos, int largo) {
    uint8_t crc = 0x00;
    for (int i = 0; i < largo; i++) {
        crc ^= datos[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

// Rellena la firma de un paquete (lo usa el semáforo antes de llenar los campos)
static inline void firmarPaqueteSemaforo(estado_semaforo* paquete) {
    memcpy(paquete->firma, FIRMA_SEMAFORO, FIRMA_SEMAFORO_LARGO);
}

// Calcula el CRC con los campos ya llenos; es lo último antes de enviar
static inline void sellarPaqueteSemaforo(estado_semaforo* paquete) {
    paquete->verificacion = crc8Semaforo((const uint8_t*)paquete,
                                         (int)sizeof(estado_semaforo) - 1);
}

// true si los bytes recibidos son un paquete válido del semáforo ACPI: tamaño
// exacto, firma, CRC correcto y campos dentro de rango (lo usa el brazalete al
// recibir). Cualquier otra cosa se descarta.
static inline bool esPaqueteSemaforo(const uint8_t* datos, int largo) {
    if (datos == nullptr || largo != (int)sizeof(estado_semaforo)) return false;
    if (memcmp(datos, FIRMA_SEMAFORO, FIRMA_SEMAFORO_LARGO) != 0) return false;
    if (crc8Semaforo(datos, largo - 1) != datos[largo - 1]) return false;
    estado_semaforo p;
    memcpy(&p, datos, sizeof(p));
    return p.grados < 360 && p.permitidoCruce <= 1;
}

#endif // PROTOCOLO_ESP_H
