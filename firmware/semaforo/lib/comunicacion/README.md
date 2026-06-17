# Módulo: Comunicador Pulsera (Transmisor ESP-NOW)

## Descripción General
Este submódulo configura el microcontrolador del semáforo como un enrutador emisor de la red **ESP-NOW**. Se asegura de transmitir tramas de datos empacadas y estandarizadas (carga útil o Payload) a través de canales Wi-Fi operando en capa de enlace MAC, garantizando tiempos de respuesta por debajo de los 10 milisegundos.

---

## Contenido
- `comunicador_pulsera.h` -> Interfaz estática de la clase constructora.
- `comunicador_pulsera.cpp` -> Implementación con el Framework nativo Espressif para el envío.

---

## Explicación
- **Broadcast Incondicional**: En lugar de enlazar el semáforo con la dirección MAC específica de cada pulsera de un peatón invidente en la ciudad, el módulo utiliza una dirección `BROADCAST_MAC` predefinida universalmente (`FF:FF:FF:FF:FF:FF`). 
- **Concurrencia**: Cualquier pulsera que entre al rango de cobertura del semáforo (aprox. 10 a 50 metros dependiendo de las antenas) recibe pasivamente la información sin consumir tiempo adicional de procesamiento ni provocar saturación de Handshake TCP/IP.
- El módulo se autoconfigura al iniciar encendiendo la antena interna del ESP32 en modo `WIFI_STA` (Estación) y registrando el *peer* (par destino) de broadcast sin necesidad de contraseñas de encriptación WPA.

---

## Dependencias
- Librería del protocolo TCP/IP de Espressif: `<WiFi.h>`.
- Librería de bajo nivel para enlaces de red sin conexión de Espressif: `<esp_now.h>`.
- Archivo compartido de definición de trama: `lib/protocolo/protocolo_esp.h`.

---

## Ejemplo de Uso

```cpp
#include "comunicacion/comunicador_pulsera.h"

void setup() {
    // Encender antena de forma pasiva (WIFI_STA invisible) y configurar ESP-NOW
    ComunicadorPulsera::inicializar();
}

void transmitirEvento() {
    estado_semaforo dataAEnviar;
    dataAEnviar.id = 1; // ID de Semáforo
    dataAEnviar.grados = ControladorBrujulaSemaforo::headingActual;
    dataAEnviar.permitidoCruce = 1; // 1 = Verde para peatón
    dataAEnviar.tiempo = 15; // Faltan 15 segundos para luz roja peatonal

    // Despachar el Payload al aire
    ComunicadorPulsera::enviarEstado(dataAEnviar);
}
```
