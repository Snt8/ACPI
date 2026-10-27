# Módulo: Utilidades Globales (Utils)

## Descripción General
Este submódulo centraliza las funciones matemáticas de circunferencia que usan la FSM
y el evaluador.

---

## Contenido
- `calcular_direccion.*` -> Matemáticas de arco y geometría de círculo. Sólo usa `<math.h>`, así que también compila en las pruebas de la PC (`test/`).

Los patrones de vibración ya no están aquí: viven en `src/vibracion/` (reproducción) y
`src/control/evaluador_semaforo.*` (decisión).

---

## Explicación

### 1. Giro hacia el cruce: `calcularDireccion`
Debido a que el campo magnético trabaja en forma circular $[0, 360)$, la aritmética tradicional falla (ej: la diferencia geométrica entre $350^{\circ}$ y $10^{\circ}$ no es 340, sino 20 grados). La función `calcularDireccion` utiliza `fmodf()` para encontrar la desviación angular usando aritmética modular y retornar el giro más corto en $(-180, 180]$ (positivo = hacia la derecha, negativo = hacia la izquierda).

### 2. Distancia a quedar de frente: `desviacionEnfrentamiento`
El brazalete y el semáforo están bien orientados cuando se **enfrentan**, es decir,
cuando la diferencia entre sus headings es de unos $180^{\circ}$:

$$\Delta\theta = \min(|H_p - H_s|,\ 360 - |H_p - H_s|) \qquad \text{desviación} = |\Delta\theta - 180^{\circ}|$$

Devuelve 0 cuando están de frente y 180 cuando miran hacia el mismo lado. El
`EvaluadorSemaforo` la compara con `UMBRAL_CRUCE_GRADOS` (con histéresis).

---

## Dependencias
- `constantes.h` (`GRADOS_CIRCUNFERENCIA`).

---

## Ejemplo de Uso
```cpp
#include "utils/calcular_direccion.h"

float hp = 300.0f;   // heading del brazalete
float hs = 90.0f;    // heading del semáforo
float desviacion = desviacionEnfrentamiento(hp, hs);         // 30°
float giro = calcularDireccion(fmodf(hs + 180.0f, 360.0f), hp); // -30°: girar a la izquierda
```
