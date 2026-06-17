# Módulo: Comunicaciones Inalámbricas (Pulsera)

## Descripción General
Este directorio contiene las capas de abstracción para la topología de red de doble enlace requerida por la Pulsera ACPI. El ESP32-C3 gestiona dos canales de radio de forma simultánea pero asíncrona: **ESP-NOW** para recibir comandos inmediatos desde el semáforo de infraestructura, y **Bluetooth Low Energy (BLE)** para actuar como servidor pasivo frente al Smartphone Android.

---

## Contenido
El módulo se divide en dos sub-módulos críticos aislados:
- `/movil/` (`movil.h`, `movil.cpp`) -> Implementación de un Servidor BLE Periférico usando **NimBLE**.
- `/semaforo/` (`semaforo.h`, `semaforo.cpp`) -> Implementación del receptor asíncrono broadcast **ESP-NOW**.

---

## Explicación

### 1. Comunicación con el Móvil (BLE)
Para maximizar la autonomía, la pulsera emite un **Servicio GATT** con características configuradas en modo *Notify* (Transmisión) y *Write* (Recepción). 
En lugar de enviar un flujo continuo de bytes incomprensibles, la pulsera envía telemetría compacta en JSON plano cada medio segundo, conteniendo el Heading (`hd`), Estado de Máquina (`st`), Alineación (`ok`), Tiempo del Semáforo (`tr`), y Batería (`bt`).
```json
{"hd":275,"st":0,"ok":true,"tr":15,"bt":-1}
```

### 2. Comunicación con el Semáforo (ESP-NOW)
ESP-NOW permite conectividad sin enrutadores *peer-to-peer* (P2P). La pulsera expone una función de Callback (`onDatosRecibidos`) que se ejecuta a nivel de Interrupción de Hardware en el procesador.
Al recibir el empaquetado estructurado (`estado_semaforo`), los bytes se transcriben en memoria y se reportan inmediatamente a la Máquina de Estados para resetear los temporizadores de Suspensión (Sleep).

---

## Dependencias
- Biblioteca externa: `h2zero/NimBLE-Arduino` (Optimizada para arquitecturas con memoria limitada como ESP32-C3).
- Bibliotecas internas de Espressif: `<WiFi.h>` y `<esp_now.h>`.
- Módulo interno: `lib/protocolo/` (Define las estructuras de red).

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
