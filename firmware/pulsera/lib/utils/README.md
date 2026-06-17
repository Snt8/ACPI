# Módulo: Utilidades Globales (Utils)

## Descripción General
Este submódulo centraliza funciones y abstracciones matemáticas complejas, calculadoras angulares de circunferencia y gestores de temporización avanzada asíncrona para descargar de lógica *spaghetti* a la FSM principal de la pulsera.

---

## Contenido
- `calcular_direccion.*` -> Matemáticas de arco y geometría de círculo.
- `patrones_vibracion.*` -> Manejadores de temporizadores (`millis`) asíncronos para generar feedback háptico rítmico.
- `indicar_error.*` -> Rutinas críticas de bloqueo (Watchdog manuales).

---

## Explicación

### 1. Cálculos de Dirección Diferencial
Debido a que el campo magnético trabaja en forma circular $[0, 360)$, la aritmética tradicional falla (ej: la diferencia geométrica entre $350^{\circ}$ y $10^{\circ}$ no es 340, sino 20 grados). La función `calcularDireccion` utiliza `fmod()` para encontrar la desviación angular usando aritmética modular y retornar el vector más corto (positivo = hacia la derecha, negativo = hacia la izquierda).

### 2. Patrones de Vibración (FSM Internas No-Bloqueantes)
Usar `delay()` en el microcontrolador causaría la pérdida de paquetes ESP-NOW y BLE al bloquear el núcleo. 
El archivo `patrones_vibracion` implementa un generador asíncrono de perfiles de vibración:
- **Confirmatorio**: Ejecuta una coreografía (Pulso -> Pausa -> Pulso) midiendo diferencias de tiempo transcurrido, preservando los ciclos del loop principal de la pulsera.
- **Direccional**: Pulso constante supervisado (Watchdog por tiempo límite).

---

## Dependencias
- Módulo `lib/motores` (Para invocar llamadas físicas reales).
- Archivo de configuraciones generales (`constantes.h` para usar el valor de PI o Grados de Círculo).

---

## Ejemplo de Uso

**Generación asíncrona (A usar dentro del loop/FSM)**:
```cpp
#include "utils/patrones_vibracion.h"

void loop() {
    // Retorna true si la animación está en curso, sin bloquear la ejecución actual.
    // Llama a los motores internamente.
    bool ocupado = vibracionConfirmatoria(ControladorMotores::AMBOS);
    
    if(!ocupado) {
        // Ejecutar otras labores.
    }
}
```
