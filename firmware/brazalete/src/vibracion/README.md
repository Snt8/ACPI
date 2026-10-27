# Módulo: Patrones de Vibración

## Descripción General
Reproduce los patrones hápticos del brazalete definidos en `patrones.md` (raíz del
firmware). Cada patrón suena **una sola vez** cuando se lo pide, y el módulo garantiza
que entre el inicio de un patrón y el del siguiente pasen al menos 3 s, para no forzar
las soldaduras de los motores.

---

## Contenido
- `controlador_patrones.h` -> Enumeración `Patron`, estructura `SalidaMotores` y la clase `ControladorPatrones`.
- `controlador_patrones.cpp` -> Tabla de formas (pulsos, duración, pausa, motores) y la lógica de tiempos.

---

## Explicación
Todos los patrones tienen la misma forma: **N pulsos iguales separados por una pausa**,
en uno o en ambos motores. La tabla `formaDe()` describe cada uno con los valores de
`constantes.h`:

| Patrón | Motores | Pulsos | Pulso | Pausa | Total |
|---|---|---|---|---|---|
| `SEMAFORO_DETECTADO` | ambos | 3 | 200 ms | 500 ms | 1.6 s |
| `GIRAR_DERECHA` / `GIRAR_IZQUIERDA` | uno | 1 | 300 ms | — | 0.3 s |
| `CRUCE_PERMITIDO` | ambos | 2 | 2 s | 500 ms | 4.5 s |
| `CRUCE_PROHIBIDO` | ambos | 1 | 2 s | — | 2 s |

El controlador **no toca el hardware ni llama a `millis()`**: recibe el tiempo y
devuelve qué motor debe estar encendido (`SalidaMotores`). Así:
- se prueba en la PC (`test/test_controlador_patrones.cpp`);
- la FSM es el único lugar que escribe en los motores (`ControladorMotores::aplicar`).

Garantías:
- Sin `iniciar()` los motores nunca se encienden.
- Un patrón termina solo y no se repite.
- `libre()` es falso mientras suena un patrón o si no pasaron `SEPARACION_MINIMA_EVENTOS_MS`
  desde el último inicio; `iniciar()` rechaza el pedido en ese caso.
- `cancelar()` apaga el patrón, pero la separación se sigue contando desde su inicio.

Quién decide **qué** patrón pedir y **cuándo** es `EvaluadorSemaforo` (`src/control/`).

---

## Dependencias
- `constantes.h` (tiempos de cada patrón y separación mínima).
- Sin Arduino: sólo `<stdint.h>`.

---

## Ejemplo de Uso
```cpp
#include "vibracion/controlador_patrones.h"

ControladorPatrones patrones;

void loop() {
    uint32_t ahora = millis();
    if (hayEvento && patrones.libre(ahora)) {
        patrones.iniciar(Patron::SEMAFORO_DETECTADO, ahora);
    }
    SalidaMotores s = patrones.actualizar(ahora);
    ControladorMotores::aplicar(s.derecho, s.izquierdo);
}
```
