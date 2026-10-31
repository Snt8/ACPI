# Módulo: Comunicaciones Inalámbricas (Brazalete)

## Descripción General
Este directorio contiene las capas de abstracción para la topología de red de doble enlace requerida por el Brazalete ACPI. El ESP32-C3 gestiona dos canales de radio de forma asíncrona: **ESP-NOW** para recibir el estado del semáforo de infraestructura, y **Bluetooth Low Energy (BLE)** para actuar como servidor GATT frente a la app móvil (`app-movil/`, iOS y Android). Las dos radios sólo están encendidas a la vez en `SENSING_CROSSING`: en standby el BLE está apagado y el WiFi sólo se enciende en las ventanas de escucha (ver `src/control/README.md`).

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
Acepta un MTU de hasta 185 (el paquete no cabe en el de 23 por defecto; lo negocia el móvil) y atiende los comandos `TEST_ALERT` y `VIBRATE_STOP` de la app (pantalla Diagnóstico), como mucho uno cada 200 ms. Atiende un móvil a la vez y vuelve a anunciarse cuando se desconecta.

### 2. Comunicación con el Semáforo (ESP-NOW)
ESP-NOW permite conectividad sin enrutadores *peer-to-peer* (P2P). El callback `onDatosRecibidos` corre en la tarea de WiFi (no en `loop()`): sólo valida el paquete (tamaño, firma `"ACPI"`, CRC-8 y rangos) y lo deja en un buffer protegido. La FSM lo recoge desde `loop()` con `tomarPaquete()`, y es ella la que reinicia el temporizador de standby (ver `semaforo/README.md`).

---

## Dependencias
- Biblioteca `BLE` incluida en el core Arduino-ESP32 (en el ESP32-C3 usa NimBLE, optimizado para memoria limitada).
- Bibliotecas internas de Espressif: `<WiFi.h>` y `<esp_now.h>`.
- Módulo interno: `src/protocolo/` (Define las estructuras de red).

---

## Ejemplo de Uso

En el firmware las radios no se encienden en `setup()`: las enciende y apaga la FSM
(`src/control/fsm.cpp` y `modo_standby.cpp`). Lo que hace en cada caso:

```cpp
#include "comunicacion/semaforo/semaforo.h"
#include "comunicacion/movil/movil.h"

// Ventana de escucha del standby: WiFi en modo estación + ESP-NOW
ComunicadorSemaforo::inicializar();

// Al oír al semáforo (SENSING_CROSSING): se suma el BLE
ComunicadorMovil::inicializar();

// En cada vuelta de loop()
estado_semaforo paquete;
if (ComunicadorSemaforo::tomarPaquete(paquete)) {
    // paquete.id, paquete.grados, paquete.permitidoCruce, paquete.tiempo
}
ComandoMovil comando = ComunicadorMovil::tomarComando();  // TEST_ALERT / VIBRATE_STOP

// Cada 500 ms, si hay un móvil conectado (campos en paquete_telemetria.h)
TelemetriaBle t = {};
t.rumbo = 21;
t.estado = 1;
ComunicadorMovil::enviarTelemetria(t);

// De vuelta a standby: primero el BLE, después el WiFi
ComunicadorMovil::desinicializar();
ComunicadorSemaforo::desinicializar();
```
