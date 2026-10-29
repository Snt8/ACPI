# Módulo: Controlador Brújula QMC6308 (Semáforo)

## Descripción General
Este submódulo maneja la inicialización, extracción de datos crudos y conversión matemática del sensor magnetómetro (QMC6308) instalado permanentemente de forma estática sobre el semáforo. Sirve para inyectar los grados de inclinación fija en los paquetes de telemetría emitidos.

---

## Contenido
- `controlador_brujula.h` -> Definición de la clase estática de control, métodos I2C y almacenador de offsets.
- `controlador_brujula.cpp` -> Lógica operativa del bus Wire, inyección de modo de trabajo y filtros iterativos.

---

## Explicación
A diferencia del brazalete, el magnetómetro del semáforo:
- **Carece de Compensación de Inclinación (Tilt Compensation)**: Al instalarse firmemente apuntando hacia la línea de cebra (andén peatonal), el semáforo no oscilará ni se inclinará, obviando la necesidad costosa de tener un giroscopio acoplado. El rumbo se calcula directamente con $atan2(y, x)$.
- **Calibración _Hard-Iron_ guardada**: el campo fijo de la propia placa se mide **una vez**, girando el semáforo plano al menos una vuelta antes de fijarlo, y queda en la memoria no volátil. Se ajusta un círculo a las lecturas (`calibracion_magnetica.*`, copia idéntica de la del brazalete) y su centro se resta. Al arrancar sin calibración se dan 30 s para hacerla (`CALIBRACION_AL_ARRANCAR_MS`); si no se completa, envía el rumbo sin calibrar. Con `c` por el monitor serie se borra y se calibra de nuevo (90 s máximo).
- **Promedio**: el semáforo no se mueve, así que envía el promedio circular de las últimas 10 lecturas (5 s), redondeado al grado.
- Antes había un *Leaky Integrator* que restaba el promedio de la propia lectura: con el semáforo quieto borraba también el campo de la Tierra y a los ~4 minutos los grados enviados eran aleatorios.

Secuencia de inicialización en hardware real:
1. `Soft Reset` inyectado al registro `0x0D`.
2. Activación de `Modo Continuo a 200 Hz` en el registro `0x0A`.
3. Lectura recurrente `LSB-first` de los 6 bytes a partir del registro de salida `0x01`.

---

## Dependencias
- Biblioteca nativa `<Wire.h>`.
- `<math.h>` para `atan2f()`.
- `Preferences.h` del core (memoria no volátil) para guardar la calibración.
- Constantes de calibración y promedio (`CALIBRACION_*`, `LECTURAS_PROMEDIO`) en `src/config.h`.

---

## Ejemplo de Uso

Inicializar el chip I2C en el Setup y luego obtener lecturas en el Bucle de forma temporizada:

```cpp
#include "brujula/controlador_brujula.h"

void setup() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    ControladorBrujulaSemaforo::inicializar();
}

void loop() {
    // Retorna true si el sensor es capaz de contestar
    if(ControladorBrujulaSemaforo::leerHeading()) {
        float orientacion = ControladorBrujulaSemaforo::headingActual;
        // ... Empaquetar y enviar por ESP-NOW
    }
}
```
