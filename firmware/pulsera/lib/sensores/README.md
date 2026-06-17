# Módulo: Controladores I2C de Bajo Nivel (Sensores)

## Descripción General
Este submódulo concentra los drivers de bajo nivel que interactúan físicamente a través del protocolo **I2C** con los circuitos integrados (*Sensors*) instalados en la pulsera: el acelerómetro/giroscopio inercial de 6 ejes **MPU6050** y el compás digital de 3 ejes **QMC6308**.

---

## Contenido
- `mpu6050.h` / `mpu6050.cpp` -> Abstracción del Acelerómetro/Giroscopio (Dirección `0x68`).
- `qmc5883l.h` / `qmc5883l.cpp` -> Abstracción del Magnetómetro QMC6308 (Dirección `0x2C`).

---

## Explicación

### 1. Acelerómetro (MPU6050)
Se inicializa escribiendo el bit de _wake-up_ en el registro `PWR_MGMT_1` (`0x6B`). 
Al solicitar una lectura, lee secuencialmente 6 bytes (registros a partir del `0x3B`), que representan los vectores de aceleración tridimensional. Puesto que el chip se configura por defecto al rango $\pm2g$, se dividen los datos brutos por un `FACTOR_ESCALA` constante (`16384 LSB/g`) estipulado en el datasheet para devolver los vectores limpios ($g_x, g_y, g_z$).

### 2. Magnetómetro (QMC6308)
*(Nota: Históricamente nombrado `qmc5883l` en el código, pero el driver interno se reescribió para QMC6308).*
Al iniciar, inyecta un *Soft Reset* y se configura en modo de operación continua a 200 Hz (`0x03` al reg `0x0A`).
Incluye un algoritmo iterativo de **Filtro Leaky Integrator** para autocalibración de _Hard-Iron_:
```cpp
offset_x = (offset_x * 0.99f) + (lectura_actual_x * 0.01f);
x_corregida = lectura_actual_x - offset_x;
```
Posteriormente, implementa el algoritmo complejo de **Tilt Compensation**. Dado que un compás se deforma al estar inclinado, utiliza los vectores $g$ limpios del MPU6050 para rotar la matriz magnética de forma tridimensional antes de calcular el Azimuth / Heading.

---

## Dependencias
- Biblioteca Estándar **Wire.h** de Arduino para las transferencias asíncronas vía Bus I2C.
- `<math.h>` para algoritmos trigonométricos avanzados (`atan2f`, `sqrtf`).

---

## Ejemplo de Uso

El módulo se usa exclusivamente a través de la capa superior (`lib/brujula`), nunca desde la Máquina de Estados directamente.

```cpp
#include "sensores/mpu6050.h"
#include "sensores/qmc5883l.h"

// Inicializar ambos nodos I2C
ControladorAcelerometro::inicializar();
ControladorMagnetometro::inicializar();

// Leer vectores de gravedad
ControladorAcelerometro::leerDatos();
float gx = ControladorAcelerometro::getGx();
float gy = ControladorAcelerometro::getGy();
float gz = ControladorAcelerometro::getGz();

// Leer Norte Magnético, autocalibrar y aplicar inclinación
ControladorMagnetometro::leerPosicion();
float orientacion = ControladorMagnetometro::calcularHeading(gx, gy, gz);
```