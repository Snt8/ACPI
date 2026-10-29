# Módulo: Comunicaciones Inalámbricas (Brazalete)

## Descripción General
Este directorio contiene las capas de abstracción para la topología de red de doble enlace requerida por el Brazalete ACPI. El ESP32-C3 gestiona dos canales de radio de forma simultánea pero asíncrona: **ESP-NOW** para recibir comandos inmediatos desde el semáforo de infraestructura, y **Bluetooth Low Energy (BLE)** para actuar como servidor pasivo frente al Smartphone Android.

---

## Contenido
El módulo se divide en dos sub-módulos críticos aislados:
- `/movil/` (`movil.h`, `movil.cpp`) -> Implementación de un Servidor BLE Periférico usando la biblioteca **BLE** del core (sobre NimBLE en el ESP32-C3).
- `/semaforo/` (`semaforo.h`, `semaforo.cpp`) -> Implementación del receptor asíncrono broadcast **ESP-NOW**.

---

## Explicación

### 1. Comunicación con el Móvil (BLE)
Para maximizar la autonomía, el brazalete emite un **Servicio GATT** con características configuradas en modo *Notify* (Transmisión) y *Write* (Recepción). 
En lugar de enviar un flujo continuo de bytes incomprensibles, el brazalete envía telemetría compacta en JSON plano cada medio segundo (`paquete_telemetria.cpp`, versión 2): rumbo (`hd`), estado del cruce (`st`), alineación (`ok`), tiempo de la luz (`tr`) y batería (`bt`), más secuencia, semáforo, luz, giro, estado de la brújula, patrón en curso y recepción y señal del semáforo. El contrato completo está en `docs/ble-protocol.md`.
```json
{"v":2,"n":412,"hd":21,"st":1,"ok":true,"tr":18,"bt":-1,"id":1,"lz":1,"hs":200,"gr":-1,"br":1,"pt":4,"rx":20,"rs":-67}
```
Pide un MTU de 185 (el paquete no cabe en el de 23 por defecto) y atiende los comandos `TEST_ALERT` y `VIBRATE_STOP` de la app (pantalla Diagnóstico).

### 2. Comunicación con el Semáforo (ESP-NOW)
ESP-NOW permite conectividad sin enrutadores *peer-to-peer* (P2P). El brazalete expone una función de Callback (`onDatosRecibidos`) que se ejecuta a nivel de Interrupción de Hardware en el procesador.
Al recibir el empaquetado estructurado (`estado_semaforo`), los bytes se transcriben en memoria y se reportan inmediatamente a la Máquina de Estados para resetear los temporizadores de Suspensión (Sleep).

---

## Dependencias
- Biblioteca `BLE` incluida en el core Arduino-ESP32 (en el ESP32-C3 usa NimBLE, optimizado para memoria limitada).
- Bibliotecas internas de Espressif: `<WiFi.h>` y `<esp_now.h>`.
- Módulo interno: `src/protocolo/` (Define las estructuras de red).

---

## Ejemplo de Uso

Inicialización dual en el boot del microcontrolador:

```cpp
#include "comunicacion/semaforo/semaforo.h"
#include "comunicacion/movil/movil.h"

void setup() {
    // Inicializar el radio Wi-Fi en modo Estación (necesario para ESP-NOW)
    ComunicadorSemaforo::inicializar();
    
    // Inicializar la pila del radio Bluetooth LE
    ComunicadorMovil::inicializar();
}

void loop() {
    // El receptor ESP-NOW es un callback automático invisible al loop.
    
    // Para notificar un nuevo estado al celular conectado:
    ComunicadorMovil::enviarDatos(
        headingActual, 
        estadoFSM, 
        orientacionCorrecta, 
        tiempoSemaforo, 
        nivelBateria
    );
}
```
