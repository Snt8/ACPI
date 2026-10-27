# Módulo: Controlador Lógico de Brújula (Brazalete)

## Descripción General
Este módulo es una capa de abstracción de alto nivel (`ControladorBrujula`) que se encarga de orquestar la lectura concurrente de los controladores físicos de bajo nivel (acelerómetro y magnetómetro), realizar la fusión sensorial (*Sensor Fusion*) para la compensación de inclinación (*Tilt Compensation*) y ejecutar los filtros de seguridad direccional.

---

## Contenido
- `brujula.h` -> Definiciones de la interfaz y estados estáticos de seguridad.
- `brujula.cpp` -> Implementación de la fusión sensorial y del safety check.

---

## Explicación
El módulo extrae los datos vectoriales brutos de los sensores I2C y ejecuta dos tareas fundamentales:

1. **Heading con Tilt Compensation**: combina acelerómetro y magnetómetro para obtener el rumbo aunque la muñeca esté inclinada.
2. **Safety Check (Filtro Antiruido)**: El módulo monitorea constantemente los cambios bruscos de orientación. Si el brazo del usuario oscila repentinamente y el rumbo cambia más de `UMBRAL_DELTA_HEADING` (e.g. 40°) entre dos muestras seguidas (`UMBRAL_TIEMPO_SAFETY_MS`, el doble del intervalo de muestreo), la lectura se marca como **inválida**. Con una lectura inválida el `EvaluadorSemaforo` no indica giros.

Saber si la persona está **enfrentada** al semáforo ya no es tarea de este módulo: la
desviación se calcula con `desviacionEnfrentamiento()` (`src/utils/calcular_direccion.h`)
y la decisión, con histéresis, la toma `EvaluadorSemaforo` (`src/control/`).

La FSM no lee la brújula mientras vibra un motor: los imanes de los motores alteran el
magnetómetro.

---

## Dependencias
- Módulos internos: `src/sensores` (MPU6500, manejado por el driver `mpu6050`, y QMC5883L).
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
    // 3. ¿Cuánto falta para quedar de frente al semáforo?
    float desviacion = desviacionEnfrentamiento(ControladorBrujula::headingActual,
                                                rumboSemaforoRecibido);
}
```
