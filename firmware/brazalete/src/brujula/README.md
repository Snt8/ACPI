# Módulo: Controlador Lógico de Brújula (Brazalete)

## Descripción General
Este módulo es una capa de abstracción de alto nivel (`ControladorBrujula`) que se encarga de orquestar la lectura concurrente de los controladores físicos de bajo nivel (acelerómetro y magnetómetro), realizar la fusión sensorial (*Sensor Fusion*) para la compensación de inclinación (*Tilt Compensation*) y ejecutar los filtros de seguridad direccional.

---

## Contenido
- `brujula.h` -> Definiciones de la interfaz y estados estáticos de seguridad.
- `brujula.cpp` -> Implementación de la fusión sensorial, la calibración guardada y el safety check.
- `calibracion_magnetica.h` / `.cpp` -> Ajuste de esfera para la calibración _Hard-Iron_ (sin Arduino; se prueba en `test/`). El semáforo tiene una copia idéntica.

---

## Explicación
El módulo extrae los datos vectoriales brutos de los sensores I2C y ejecuta estas tareas:

1. **Promedio**: cada lectura promedia 4 muestras de cada sensor (`MUESTRAS_POR_LECTURA`).
   Si el acelerómetro no mide ~1 g (±0.15 g), la muñeca se está moviendo y la lectura se
   descarta.
2. **Calibración _Hard-Iron_**: resta el campo fijo del propio brazalete (los imanes de los
   motores lo desvían hasta ~40°). Ver la sección siguiente.
3. **Heading con Tilt Compensation** (`rumboCompensado()` en `src/utils/`): rumbo del eje X
   del magnetómetro proyectado en el plano horizontal. La fórmula anterior proyectaba el
   campo y tomaba `atan2(y, x)` en el marco del sensor: con la muñeca a 30°/30° fallaba 16°.
   Al final se suma `CORRECCION_MONTAJE_GRADOS` (`config.h`).
4. **Safety Check (Filtro Antiruido)**: El módulo monitorea constantemente los cambios bruscos de orientación. Si el brazo del usuario oscila repentinamente y el rumbo cambia más de `UMBRAL_DELTA_HEADING` (e.g. 40°) entre dos muestras seguidas (`UMBRAL_TIEMPO_SAFETY_MS`, el doble del intervalo de muestreo), la lectura se marca como **inválida**. Con una lectura inválida el `EvaluadorSemaforo` no indica giros.

Saber si la persona está **enfrentada** al semáforo ya no es tarea de este módulo: la
desviación se calcula con `desviacionEnfrentamiento()` (`src/utils/calcular_direccion.h`)
y la decisión, con histéresis, la toma `EvaluadorSemaforo` (`src/control/`).

La FSM no lee la brújula mientras vibra un motor: los imanes de los motores alteran el
magnetómetro.

### Calibración

Se hace **una vez** y queda guardada en la memoria no volátil (sobrevive a reinicios y a
cargar un programa nuevo). Hay que repetirla si cambian los motores, la carcasa o la
posición de la placa.

1. Con el monitor serie abierto, arrancar el brazalete sin calibración guardada (la primera
   vez) o enviar `c` (borra la calibración y reinicia).
2. Durante el arranque aparece `[CALIBRACION] ...`: girar el brazalete en todas
   direcciones, también boca abajo, como dibujando un 8, lejos de objetos metálicos y del
   celular. Cada segundo se imprime cuánto recorrió cada eje (`X`, `Y`, `Z`) y el mínimo
   que hace falta (`CALIBRACION_SPAN_MINIMO_LSB`).
3. Cuando los tres ejes llegan al mínimo aparece `LISTA, sigue girando`; tras 5 s sin
   mejorar termina sola y muestra `Lista: centro (...) LSB, guardada`.
4. Si en 90 s no se completa, el brazalete arranca **sin calibrar**: suena "semáforo
   detectado" pero no indica giros ni la luz (la orientación no sería fiable).

El centro se obtiene ajustando una esfera a todas las lecturas por mínimos cuadrados, no
con (máx + mín) / 2: girándolo a mano ningún eje llega justo a su extremo y eso dejaba ~2°
de error. En simulación, tras calibrar, el rumbo queda con menos de 0.6° de error con la
muñeca plana o inclinada hasta 30°/45°.

### Ejes del acelerómetro (se detectan solos)

En la PCB v3.5 el módulo MPU está rotado 270° y el GY-271 90°: si sus ejes no coinciden,
la compensación de inclinación empuja hacia el lado contrario y el rumbo cambia decenas
de grados al mover la muñeca. Durante la calibración se guardan también lecturas del
acelerómetro y se prueban las 8 formas posibles de montarlo (`ejes_acelerometro.*`): la
correcta es la que deja constante el ángulo entre la gravedad y el campo de la Tierra.
Se imprime `[CALIBRACION] Acelerometro girado ... respecto al magnetometro` y se guarda
junto con la calibración. `ROTACION_ACELEROMETRO_GRADOS` (`config.h`) sólo se usa si no
hay una detección guardada. La clave de la calibración pasó a `cal_v2`: la primera vez
que arranque con este firmware pide calibrar de nuevo.

### Comprobaciones con el brazalete puesto

- **Inclinación**: con el brazalete quieto, inclinar la muñeca ~30° hacia cada lado sin
  girar. `[HDG] Brazalete=` debe cambiar menos de 2-3°.
- **Montaje** (`CORRECCION_MONTAJE_GRADOS`): de pie, mirando en la misma dirección que la
  brújula del celular, comparar con `[HDG] Brazalete=`; la corrección es celular − brazalete.

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
