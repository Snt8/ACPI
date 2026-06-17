# Módulo: Máquina de Estados Finita - FSM (Pulsera)

## Descripción General
Este submódulo implementa el núcleo lógico de ejecución de la Pulsera ACPI mediante una **Máquina de Estados Finita (FSM)**. Es responsable de manejar las transiciones de estado del sistema (Arranque, Suspensión, Muestreo de Cruce y Pánico), orquestando las respuestas de los motores y el envío de telemetría a un ciclo bloqueado de **2 Hz**.

---

## Contenido
- `fsm.h` -> Definición de la clase `MaquinaEstados` y la enumeración `EstadoPulsera`.
- `fsm.cpp` -> Implementación del ciclo de vida del *loop* principal, manejo de *Wake-ups* y rutinas ISR de botones.

---

## Explicación
El dispositivo Wearable tiene fuertes restricciones de consumo energético. La FSM controla estrictamente qué partes del código se ejecutan dependiendo del contexto:

- **`BOOTING`**: Estado transitorio al arrancar. Inicia periféricos y salta inmediatamente a Standby.
- **`STANDBY_LOW_POWER`**: Estado por defecto. Si no hay conexión Bluetooth activa ni se han detectado semáforos por ESP-NOW en 30 segundos, ordena al ESP32-C3 entrar en `esp_light_sleep_start()`, apagando los radios y la CPU hasta que ocurra un evento externo (interrupción de GPIO).
- **`SENSING_CROSSING`**: Estado activo al estar cerca de un semáforo. Restringe todos los cálculos matemáticos (brújula y motores) a una comprobación temporal estricta de **2 Hz (cada 500 ms)** para no agotar la CPU. Define qué patrón de vibración aplicar (Confirmatorio o Direccional) consultando el módulo de Brújula.
- **`ALERT_PANIC`**: Activado mediante una Interrupción de Hardware (ISR) al presionar el Botón BOOT (GPIO 9). Sobrescribe cualquier estado, activa un patrón de vibración intermitente ultrarrápido y alerta al Smartphone de forma inmediata.

---

## Dependencias
- Módulos internos acoplados: `lib/motores`, `lib/brujula`, `lib/comunicacion`, `lib/utils`.
- Framework ESP-IDF: `<esp_sleep.h>` y `<driver/gpio.h>` para manejo de bajo consumo.

---

## Ejemplo de Uso

La FSM se aloja puramente en `main.cpp` liberando al desarrollador de crear sub-bucles infinitos.

```cpp
#include "control/fsm.h"

void setup() {
    // Tras iniciar los buses I2C y periféricos...
    MaquinaEstados::inicializar();
}

void loop() {
    // La FSM evalúa millis() y resuelve lógicas internamente
    // sin el uso de bloqueos delay()
    MaquinaEstados::actualizar();
}
```
