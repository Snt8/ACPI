# Módulo: Protocolo de Red Compartido (Semáforo)

## Descripción General
Archivo de definición del contrato C/C++ (`struct`) de la carga útil (*Payload*) que el semáforo inyecta en el flujo de red ESP-NOW. Es idéntico y copiado estrictamente al que se aloja en la infraestructura del receptor (Pulsera) para evitar desajustes en el *Parsing* de memoria.

---

## Contenido
- `protocolo_esp.h` -> Estructura plana de 5 Bytes sin _Padding_.

---

## Explicación
Se define un marco de memoria continuo utilizando el empaquetado del compilador GCC `__attribute__((packed))` compuesto de:

1. `id` (`uint8_t`): Identificador único del nodo semafórico en caso de tener múltiples postes emitiendo simultáneamente en una intersección grande.
2. `grados` (`uint16_t`): Almacenamiento trunco desde el float real del magnetómetro. Abarca del $0$ al $359$ con un peso de dos bytes.
3. `permitidoCruce` (`uint8_t`): Señal de bandera booleana (`1` o `0`).
4. `tiempo` (`uint8_t`): Valor en segundos restantes para cambio de estado en la FSM del Semáforo.

Esta compactación no sólo reduce drásticamente las latencias de radio, sino que también mejora sustancialmente el consumo de corriente al despertar por menos milisegundos la antena TX del dispositivo.

---

## Dependencias
- `<stdint.h>`.
- Copia literal en ambos espacios de trabajo (Pulsera/Semáforo).

---

## Ejemplo de Uso
(Ver la implementación general de `lib/comunicacion` en ambos proyectos).
