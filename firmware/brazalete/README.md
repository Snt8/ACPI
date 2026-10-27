# Firmware Brazalete (Wearable ACPI)

## Descripción General
Este es el código fuente principal del **Nodo Maestro (Brazalete)** del proyecto ACPI. Diseñado para correr sobre un microcontrolador **ESP32-C3 Super Mini**, actúa como un procesador _Edge Computing_ portátil y de ultrabajo consumo. Es responsable de recopilar lecturas inerciales y magnéticas, evaluar la orientación peatonal contra las señales del semáforo, emitir respuestas físicas (hápticas) al usuario y transmitir los resultados de telemetría al smartphone.

Se compila con **Arduino IDE** o con **ESP-IDF**; ya no necesita PlatformIO.

---

## Contenido
- `brazalete.ino` -> Punto de entrada (`setup()` / `loop()`). Es el archivo que se abre en Arduino IDE.
- `src/` -> Todo el resto del código; Arduino IDE lo compila solo.
  - `config.h` -> **Pines y direcciones I2C** del hardware (lo primero que hay que revisar).
  - `constantes.h` -> Constantes matemáticas, intensidades y tiempos de vibración.
  - `brujula/`, `sensores/`, `motores/`, `control/`, `comunicacion/`, `protocolo/`, `utils/` -> Módulos.
- `main/`, `CMakeLists.txt`, `sdkconfig.defaults` -> Archivos de ESP-IDF (Arduino como componente). Arduino IDE los ignora.
- `pruebas/` -> Sketches independientes para validar el hardware (ver abajo).

---

## Hardware (PCB v3.5)
| Señal | GPIO |
|---|---|
| I2C SDA | 8 |
| I2C SCL | 9 |
| Motor derecho | 10 |
| Motor izquierdo | 3 |

Sensores (confirmados con la prueba 02): **MPU6500** en 0x68 (acelerómetro/giroscopio; el módulo se vende como MPU9250 pero no trae magnetómetro) y GY-271 con **QMC5883L** en 0x0D.
Las direcciones reales se confirman con las pruebas 01 y 02 y se ponen en `src/config.h`.

> El driver del magnetómetro (`src/sensores/qmc5883l.cpp`) sólo funciona con el **QMC5883L**. Si la prueba 02 encuentra otro chip, hay que adaptarlo.

---

## Explicación
El firmware está construido bajo un enfoque modular y orientado a eventos discretos, implementando una **Máquina de Estados Finita (FSM)**.

En su estado nominal de `SENSING_CROSSING`, el brazalete ejecuta un ciclo de muestreo a **2 Hz (cada 500 ms)**:
1. Lee los componentes vectoriales $g_x, g_y, g_z$ del acelerómetro.
2. Extrae las lecturas crudas del magnetómetro y les aplica compensación _Hard-Iron_ mediante un _Leaky Integrator_.
3. Compensa el compás magnético por inclinación (_Tilt Compensation_) para obtener el Heading.
4. Aplica una barrera de movimiento brusco (_Safety Check_): ignora lecturas si el Heading giró $>40^{\circ}$ en medio segundo.
5. Evalúa la diferencia frente al rumbo transmitido por el Semáforo.
6. Enciende los micro-motores usando rutinas no bloqueantes.

---

## Dependencias
Ninguna biblioteca externa: todo viene con el paquete **esp32 by Espressif Systems** (v3.x).
- **Wire (I2C)**: comunicación con los sensores.
- **BLE**: la biblioteca del core; en el ESP32-C3 corre sobre NimBLE (ligero en RAM y Flash).
- **ESP-NOW**: protocolo sin conexión de Espressif para recibir al semáforo.

El código también compila con el core 2.x (los cambios de API de LEDC y ESP-NOW están cubiertos con `#if`), pero se prueba con el 3.x.

---

## Compilar con Arduino IDE
1. **Herramientas → Placa → Gestor de placas**: instalar *esp32 by Espressif Systems* (3.x).
2. Abrir `firmware/brazalete/brazalete.ino`.
3. Configurar en **Herramientas**:
   - Placa: **ESP32C3 Dev Module**
   - USB CDC On Boot: **Enabled** (sin esto el monitor serie no muestra nada)
   - Partition Scheme: la de por defecto alcanza (el firmware ocupa ~94 % de 1.2 MB). Si crece, usar **Huge APP**.
   - Si la placa se reinicia en bucle después de cargar, probar Flash Mode: **DIO**.
4. Subir y abrir el monitor serie a **115200** baudios.

Lo mismo desde la terminal con `arduino-cli`:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc firmware/brazalete
```

## Compilar con ESP-IDF
Requiere **ESP-IDF v5.5** (la versión sobre la que está construido Arduino-ESP32 3.3). El componente `espressif/arduino-esp32` se descarga solo la primera vez (`main/idf_component.yml`).
```bash
cd firmware/brazalete
idf.py set-target esp32c3
idf.py build flash monitor
```
`main/main.cpp` sólo incluye `brazalete.ino`, así que ambos entornos compilan exactamente el mismo código.

---

## Pruebas de hardware (`pruebas/`)
Sketches independientes (no usan `src/`); se abren en Arduino IDE con la misma configuración de placa. Repiten su salida periódicamente, así que se puede abrir el monitor serie en cualquier momento.

| Sketch | Para qué sirve |
|---|---|
| `01_escaneo_i2c` | Lista las direcciones que responden en el bus y revisa que SDA/SCL tengan pull-up. |
| `02_identificar_sensores` | Lee el registro de ID de cada chip (distingue MPU6050/MPU6500/MPU9250, HMC5883L/QMC5883L/QMC5883P y el AK8963 interno del MPU9250) e imprime las líneas para `src/config.h`. |
| `03_prueba_sensores_motores` | Lee acelerómetro, giroscopio, temperatura y magnetómetro a 2 Hz; vibra derecho → izquierdo al arrancar y acepta comandos por serie (`d`, `i`, `a`, `b` barrido, `+`/`-`, `D`/`I` encendido fijo, `0`, `l`, `r`, `h`). |
