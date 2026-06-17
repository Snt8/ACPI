# Módulo: Controlador Lógico de Brújula (Pulsera)

## Descripción General
Este módulo es una capa de abstracción de alto nivel (`ControladorBrujula`) que se encarga de orquestar la lectura concurrente de los controladores físicos de bajo nivel (acelerómetro y magnetómetro), realizar la fusión sensorial (*Sensor Fusion*) para la compensación de inclinación (*Tilt Compensation*) y ejecutar los filtros de seguridad direccional.

---

## Contenido
- `brujula.h` -> Definiciones de la interfaz y estados estáticos de seguridad.
- `brujula.cpp` -> Implementación de algoritmos de fusión y la lógica de validación de cruce.

---

## Explicación
El módulo extrae los datos vectoriales brutos de los sensores I2C y ejecuta dos tareas fundamentales:

1. **Safety Check (Filtro Antiruido)**: El módulo monitorea constantemente los cambios bruscos de orientación. Si el brazo del usuario oscila repentinamente y el rumbo cambia más de `UMBRAL_DELTA_HEADING` (e.g. 40°) en menos de 500ms, la lectura se marca como **inválida** y se aborta el procesamiento de feedback. Esto evita que el usuario reciba vibraciones falsas por movimientos naturales de la muñeca.
2. **Evaluación Diferencial de Enfrentamiento**: A diferencia de comparar si los rumbos son iguales, este módulo usa la aritmética modular para determinar si la pulsera y el semáforo están **enfrentados** (mirándose de frente). Calcula el camino más corto en una circunferencia y evalúa la condición de tolerancia: `|Δθ - 180°| < UMBRAL_CRUCE_GRADOS`.

---

## Dependencias
- Módulos internos: `lib/sensores` (MPU6050 y QMC6308).
- Funciones matemáticas estándar (`<math.h>`).
- Constantes operativas globales (`config.h` y `constantes.h`).

---

## Ejemplo de Uso

En el loop principal de la Máquina de Estados:

```cpp
// 1. Efectuar el muestreo y ejecutar filtros matemáticos
ControladorBrujula::obtenerHeading();

// 2. Comprobar si la muñeca está estable (lectura válida)
if (ControladorBrujula::headingValido()) {
    
    // 3. Evaluar si el peatón está alineado correctamente con el semáforo
    bool estoyAlineado = ControladorBrujula::revisarSemaforo(rumboSemaforoRecibido);
    
    if(estoyAlineado) {
        // Ejecutar rutina de cruce...
    }
}
```