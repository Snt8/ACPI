# Firmware Pulsera (Wearable ACPI)

## Descripción General
Este es el código fuente principal del **Nodo Maestro (Pulsera)** del proyecto ACPI. Diseñado para correr sobre un microcontrolador **ESP32-C3 Super Mini**, actúa como un procesador _Edge Computing_ portátil y de ultrabajo consumo. Es responsable de recopilar lecturas inerciales y magnéticas, evaluar la orientación peatonal contra las señales del semáforo, emitir respuestas físicas (hápticas) al usuario y transmitir los resultados de telemetría al smartphone.

---

## Contenido
- `platformio.ini` -> Archivo de configuración de construcción de PlatformIO (Framework `espressif32@6.9.0`).
- `include/` -> Constantes globales y configuraciones macro (`config.h`, `constantes.h`).
- `src/main.cpp` -> Punto de entrada del programa.
- `lib/` -> Submódulos independientes (brújula, sensores, control, motores, comunicación, utilidades).

---

## Explicación
El firmware está construido bajo un enfoque altamente modular y orientado a eventos discretos, implementando una **Máquina de Estados Finita (FSM)**.

En su estado nominal de `SENSING_CROSSING`, la pulsera ejecuta un ciclo de muestreo estricto a **2 Hz (cada 500ms)** en el que realiza las siguientes operaciones en menos de un milisegundo de CPU, maximizando el tiempo inactivo:
1. Lee los componentes vectoriales $g_x, g_y, g_z$ del MPU6050.
2. Extrae las lecturas crudas del QMC6308 y les aplica compensación ambiental _Hard-Iron_ mediante un _Leaky Integrator_.
3. Compensa tridimensionalmente el compás magnético (_Tilt Compensation_) para obtener el Heading puro.
4. Aplica una barrera protectora de movimiento brusco (_Safety Check_): Ignora lecturas si el Heading giró $>40^{\circ}$ en medio segundo.
5. Evalúa matemáticamente la diferencia frente al rumbo transmitido por el Semáforo.
6. Enciende los micro-motores usando rutinas no-bloqueantes.

---

## Dependencias
- **NimBLE-Arduino**: Biblioteca optimizada para gestionar Bluetooth Low Energy ocupando mucho menos memoria RAM y Flash que la biblioteca oficial, ideal para el restringido ESP32-C3.
- **Wire (I2C)**: Framework nativo de Arduino para la comunicación con los sensores.
- **ESP-NOW**: Protocolo ultrarrápido sin conexión de Espressif.

---

## Ejemplo de Uso

Dado que el entorno depende de PlatformIO, la construcción de la pulsera se maneja desde la raíz del proyecto.

```bash
# Navegar a la raíz del proyecto ACPI
cd ../../

# Ejecutar el Linter de calidad de código
pio check -d firmware/pulsera

# Compilar el código
pio run -d firmware/pulsera

# Compilar y cargar en la ESP32-C3 por puerto USB
pio run -d firmware/pulsera --target upload
```
