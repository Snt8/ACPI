# Módulo: Control del Brazalete (FSM y Evaluador)

## Descripción General
Núcleo lógico del brazalete ACPI. La **Máquina de Estados Finita (FSM)** decide qué
partes del sistema están activas, y el **Evaluador de paquetes** decide cuándo y cómo
vibra el brazalete a partir de lo que envía el semáforo y de la orientación de la
persona.

---

## Contenido
- `fsm.h` / `fsm.cpp` -> Clase `MaquinaEstados` y enumeración `EstadoBrazalete`. Corre en cada `loop()`.
- `evaluador_semaforo.h` / `.cpp` -> Clase `EvaluadorSemaforo`: compara cada paquete del semáforo con el anterior, sigue la orientación y orquesta al `ControladorPatrones` (`src/vibracion/`).

---

## Explicación

### Estados de la FSM
- **`BOOTING`**: estado transitorio al arrancar; pasa enseguida a standby.
- **`STANDBY_LOW_POWER`**: sin semáforo cerca. No se leen sensores ni vibran los
  motores; se espera el primer paquete del semáforo.
- **`SENSING_CROSSING`**: hay un semáforo a la vista. En cada vuelta del `loop()`:
  1. Recoge el paquete que dejó el callback de ESP-NOW y se lo pasa al evaluador.
  2. Si pasan `TIEMPO_SIN_PAQUETES_STANDBY_MS` (30 s) sin paquetes del semáforo que se
     está siguiendo, vuelve a standby.
  3. A 2 Hz, y **sólo si no está vibrando** (los imanes de los motores alteran el
     magnetómetro), lee la brújula y le pasa al evaluador la desviación respecto al
     cruce y hacia qué lado girar.
  4. `evaluador.orquestar()`: si hay un evento pendiente y el controlador está libre,
     inicia el patrón que corresponde.
  5. Aplica a los motores la salida de `ControladorPatrones::actualizar()`.
  6. Envía la telemetría BLE al móvil cada 500 ms.

### Evaluador de paquetes (`EvaluadorSemaforo`)
Compara cada paquete con el anterior y lo clasifica:

| Resultado | Cuándo |
|---|---|
| `SEMAFORO_NUEVO` | Primer paquete desde que se reinició (al salir de standby). |
| `CAMBIO_A_PERMITIDO` / `CAMBIO_A_PROHIBIDO` | Cambió `permitidoCruce` respecto al paquete anterior. |
| `SIN_CAMBIOS` | Misma luz; sólo cambian los grados o el tiempo restante (no es un evento). |
| `IGNORADO` | Paquete de otro `id`: se sigue al primer semáforo hasta volver a standby. |

Con esos eventos y la orientación decide el patrón (ver `patrones.md`):
1. `SEMAFORO_DETECTADO` una vez por semáforo nuevo.
2. Mal orientada: `GIRAR_DERECHA` / `GIRAR_IZQUIERDA` cada `INTERVALO_GIRO_MS` (3 s).
3. De frente y con la luz sin avisar: `CRUCE_PERMITIDO` o `CRUCE_PROHIBIDO`, una vez.
4. Cualquier otro caso: nada.

"De frente" usa histéresis: se entra con desviación < 30° y se sale con > 40°. Sólo
pide patrones cuando el controlador está libre, así que la separación mínima de 3 s
entre patrones se cumple siempre.

El evaluador y el controlador no usan Arduino: sus pruebas están en `test/`.

---

## Dependencias
- `src/vibracion` (`ControladorPatrones`), `src/motores`, `src/brujula`, `src/comunicacion`, `src/utils`.
- `src/protocolo/protocolo_esp.h` (paquete `estado_semaforo`).

---

## Ejemplo de Uso
La FSM se usa desde `brazalete.ino`, sin bucles ni `delay()` propios:

```cpp
#include "src/control/fsm.h"

void setup() {
    // Tras iniciar los buses I2C y periféricos...
    MaquinaEstados::inicializar();
}

void loop() {
    MaquinaEstados::actualizar();
}
```
