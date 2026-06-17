# Módulo: Protocolo de Red Compartido

## Descripción General
Este submódulo contiene el archivo principal de estructura de red que estipula el **contrato de datos** compartido entre el dispositivo emisor (Semáforo) y el receptor (Pulsera) a través de enlaces directos (ESP-NOW).

---

## Contenido
- `protocolo_esp.h` -> Interfaz C/C++ plana (`struct`) que se envía al aire.

---

## Explicación
El protocolo ESP-NOW se basa en enviar fragmentos RAW de memoria (bytes) por broadcast o unicast. Para que ambos extremos del sistema ACPI puedan codificar y decodificar este chorro de bytes correctamente, ambos firman un contrato usando el atributo de empaquetamiento del compilador `__attribute__((packed))`.
Esto garantiza que el compilador no añada bytes de "padding" (alineación de memoria) que corromperían la lectura cruzada entre las dos placas diferentes de ESP32.

Estructura de la trama `estado_semaforo`:
- `id` (1 byte): Para direccionar múltiples semáforos si están cerca.
- `grados` (2 bytes): Orientación de infraestructura (`uint16_t`).
- `permitidoCruce` (1 byte): Estado semafórico real (1=Rojo Vehículos/Paso Peatonal, 0=Verde).
- `tiempo` (1 byte): Temporizador restante.

**Total de Payload:** 5 Bytes (Ultra eficiente).

---

## Dependencias
- Biblioteca de estándares `<stdint.h>`.
- Este archivo es **compartido de manera idéntica** con la carpeta equivalente en el firmware del Semáforo.

---

## Ejemplo de Uso

**En el Emisor (Semáforo)**:
```cpp
estado_semaforo payload;
payload.grados = 180;
payload.permitidoCruce = 1;
esp_now_send(BROADCAST_MAC, (uint8_t *) &payload, sizeof(payload));
```

**En el Receptor (Pulsera)**:
```cpp
void onDatosRecibidos(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    estado_semaforo estado_entrante;
    memcpy(&estado_entrante, data, sizeof(estado_semaforo));
    
    // estado_entrante.grados ahora es 180
}
```
