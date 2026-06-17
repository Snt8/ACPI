# Módulo: Controlador de Feedback Háptico (Motores)

## Descripción General
Este submódulo se encarga de abstraer y gestionar el control de hardware (Pines GPIO) de los micro-motores de vibración integrados en la pulsera (Izquierdo y Derecho). Provee una interfaz limpia para la Máquina de Estados para iniciar y detener las señales de PWM hacia el hardware vibratorio.

---

## Contenido
- `motor.h` -> Definiciones de pines, canales LEDC, y enumeraciones de selección.
- `motor.cpp` -> Implementación de inicialización y control de Intensidad usando el periférico LEDC del ESP32.

---

## Explicación
Debido a que la placa ESP32-C3 carece de la función `analogWrite` tradicional estándar de los microcontroladores básicos, este módulo hace uso directo de la API **LEDC (LED Control)** del ESP32, que es un generador de PWM integrado en hardware. 

- **Canales Explícitos**: Se asigna un canal dedicado (Canal 0 y Canal 1) a cada motor, corriendo a una frecuencia de $1000$ Hz y con una resolución de $8$ bits (intensidades del 0 al 255).
- La clase define la enumeración abstracta `Motor` (`DERECHO`, `IZQUIERDO`, `AMBOS`), de forma que las demás partes del código no tengan que referenciar números de pin o canales específicos.

---

## Dependencias
- **<Arduino.h>**: Específicamente la API de *espressif32* para las funciones `ledcSetup`, `ledcAttachPin` y `ledcWrite`.
- Módulo `lib/utils` (Patrones de vibración los utiliza).

--- 

## Ejemplo de Uso

```cpp
#include "motores/motor.h"

void setup() {
    if(!ControladorMotores::inicializar()){
        // Fallo crítico de hardware
    }
}

void notificarUsuarioMalOrientado() {
    // Hacemos vibrar el motor derecho a máxima potencia (255)
    ControladorMotores::vibrarMotor(ControladorMotores::DERECHO, 255);
    
    delay(500); // Demostración (en la app real usamos FSM no-bloqueante)
    
    // Apagamos ambos motores por seguridad
    ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
}
```
