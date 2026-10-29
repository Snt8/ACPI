# Módulo: Controladores I2C de Bajo Nivel (Sensores)

## Descripción General
Este submódulo concentra los drivers de bajo nivel que interactúan físicamente a través del protocolo **I2C** con los circuitos integrados (*Sensors*) instalados en el brazalete: el acelerómetro/giroscopio inercial de 6 ejes **MPU6050** y el compás digital de 3 ejes **QMC5883L** (módulo GY-271).

---

## Contenido
- `mpu6050.h` / `mpu6050.cpp` -> Abstracción del Acelerómetro/Giroscopio (Dirección `0x68`).
- `qmc5883l.h` / `qmc5883l.cpp` -> Abstracción del Magnetómetro QMC5883L (Dirección `0x0D`).

---

## Explicación

### 1. Acelerómetro (MPU6050 / MPU6500)
La placa v3.5 trae un **MPU6500** (WHO_AM_I `0x70`). El driver sirve igual: los registros que usa (`0x6B` y `0x3B`–`0x40`) y la escala de ±2g son los mismos que en el MPU6050.
Se inicializa escribiendo el bit de _wake-up_ en el registro `PWR_MGMT_1` (`0x6B`). 
Al solicitar una lectura, lee secuencialmente 6 bytes (registros a partir del `0x3B`), que representan los vectores de aceleración tridimensional. Puesto que el chip se configura por defecto al rango $\pm2g$, se dividen los datos brutos por un `FACTOR_ESCALA` constante (`16384 LSB/g`) estipulado en el datasheet para devolver los vectores limpios ($g_x, g_y, g_z$).

### 2. Magnetómetro (QMC5883L)
Al iniciar, inyecta un *Soft Reset* (`0x80` al reg `0x0A`), fija el periodo SET/RESET recomendado (`0x01` al reg `0x0B`) y se configura en modo continuo a 200 Hz, rango ±8 G (`0x1D` al reg `0x09`). Se usa ±8 G para no saturar con el imán de los motores de vibración cercanos.
Al solicitar una lectura, lee 7 bytes desde el reg `0x00` (X, Y, Z, byte bajo primero, y el registro de estado `0x06`). Si el bit OVL avisa que un eje se saturó, la lectura se descarta.
El driver sólo entrega el campo **crudo**: la calibración _Hard-Iron_ y la **Tilt Compensation** las hace `ControladorBrujula` (`src/brujula/`).

---

## Dependencias
- Biblioteca Estándar **Wire.h** de Arduino para las transferencias asíncronas vía Bus I2C.
- `<math.h>` para algoritmos trigonométricos avanzados (`atan2f`, `sqrtf`).

---

## Ejemplo de Uso

El módulo se usa exclusivamente a través de la capa superior (`src/brujula`), nunca desde la Máquina de Estados directamente.

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

// Leer el campo magnético crudo (LSB); la calibración y el rumbo los calcula
// ControladorBrujula
float crudo[3];
ControladorMagnetometro::leerCrudo(crudo);
```