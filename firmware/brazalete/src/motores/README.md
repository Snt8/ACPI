# Módulo: Controlador de Feedback Háptico (Motores)

## Descripción General
Este submódulo se encarga de abstraer y gestionar el control de hardware (Pines GPIO) de los micro-motores de vibración integrados en el brazalete (Izquierdo y Derecho). Provee una interfaz limpia para la Máquina de Estados para iniciar y detener las señales de PWM hacia el hardware vibratorio.

---

## Contenido
- `motor.h` -> Definiciones de pines, canales LEDC, y enumeraciones de selección.
- `motor.cpp` -> Implementación de inicialización y control de Intensidad usando el periférico LEDC del ESP32.

---

## Explicación
Debido a que la placa ESP32-C3 carece de la función `analogWrite` tradicional estándar de los microcontroladores básicos, este módulo hace uso directo de la API **LEDC (LED Control)** del ESP32, que es un generador de PWM integrado en hardware.

- **PWM**: $1000$ Hz con resolución de $8$ bits (intensidades del 0 al 255). Con el core 3.x el canal lo asigna el core; con el 2.x se usan los canales 0 y 1.
- **Intensidad por motor**: cada motor usa la suya, de `constantes.h`: `INTENSIDAD_MOTOR` (derecho) e `INTENSIDAD_MOTOR_IZQUIERDO`. En la PCB v3.5 los motores ven VBAT (3.7–4.2 V); a 180/255 vibraban tan fuerte que amenazaban las soldaduras, por eso hoy son 120 y 135. Se ajustan con `pruebas/03_prueba_sensores_motores`.
- **`aplicar(derecho, izquierdo)`**: deja cada motor encendido o apagado y sólo escribe en el PWM cuando el estado cambia. La FSM lo llama en cada vuelta con la salida del `ControladorPatrones`, que es quien decide cuándo vibrar (`src/vibracion/`).
- La enumeración `Motor` (`DERECHO`, `IZQUIERDO`, `AMBOS`) evita que el resto del código tenga que conocer pines o canales.

---

## Dependencias
- **<Arduino.h>**: API LEDC del core Arduino-ESP32 (`ledcAttach`/`ledcWrite` en 3.x, `ledcSetup`/`ledcAttachPin` en 2.x).
- `config.h` (pines) y `constantes.h` (intensidades).

---

## Ejemplo de Uso

```cpp
#include "motores/motor.h"

void setup() {
    if (!ControladorMotores::inicializar()) {
        // Fallo crítico de hardware
    }
}

void loop() {
    // Normalmente la salida viene de ControladorPatrones::actualizar()
    ControladorMotores::aplicar(true, false);   // sólo el derecho
    ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
}
```
