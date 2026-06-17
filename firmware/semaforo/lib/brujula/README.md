# Módulo: Controlador Brújula QMC6308 (Semáforo)

## Descripción General
Este submódulo maneja la inicialización, extracción de datos crudos y conversión matemática del sensor magnetómetro (QMC6308) instalado permanentemente de forma estática sobre el semáforo. Sirve para inyectar los grados de inclinación fija en los paquetes de telemetría emitidos.

---

## Contenido
- `controlador_brujula.h` -> Definición de la clase estática de control, métodos I2C y almacenador de offsets.
- `controlador_brujula.cpp` -> Lógica operativa del bus Wire, inyección de modo de trabajo y filtros iterativos.

---

## Explicación
A diferencia de la pulsera, el magnetómetro del semáforo:
- **Carece de Compensación de Inclinación (Tilt Compensation)**: Al instalarse firmemente apuntando hacia la línea de cebra (andén peatonal), el semáforo no oscilará ni se inclinará, obviando la necesidad costosa de tener un giroscopio acoplado. El rumbo se calcula directamente con $atan2(y, x)$.
- **Autocalibración Constante**: Debido a las distorsiones metálicas de los postes de iluminación urbanos (*Hard-Iron bias*), se incluye un **Leaky Integrator** (filtro pasa-bajas) que gradualmente detecta, absorbe y resta las desviaciones magnéticas constantes.

Secuencia de inicialización en hardware real:
1. `Soft Reset` inyectado al registro `0x0D`.
2. Activación de `Modo Continuo a 200 Hz` en el registro `0x0A`.
3. Lectura recurrente `LSB-first` de los 6 bytes a partir del registro de salida `0x01`.

---

## Dependencias
- Biblioteca nativa `<Wire.h>`.
- `<math.h>` para `atan2f()`.
- Constantes métricas de calibración `LEAKY_ALPHA` y `LEAKY_BETA` almacenadas en la carpeta de configuraciones `include/`.

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
