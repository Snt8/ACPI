# 🚦 ACPI: Asistente de Cruce Peatonal para Invidentes v3.0 👨‍🦯➡️

## Descripción General
**ACPI** es un sistema **IoT de asistencia a la movilidad** diseñado para brindar **autonomía y seguridad** a personas con discapacidad visual al cruzar calles en intersecciones semaforizadas.

El sistema combina **hardware en el borde (Edge Computing)** con sensores magnéticos e inerciales locales en un brazalete (wearable) y en un semáforo (infraestructura). Utiliza comunicación local ultrarrápida (ESP-NOW) entre nodos y telemetría por Bluetooth Low Energy (BLE) hacia una aplicación móvil integrada con **Grafana Cloud** para monitorear la seguridad de la sesión.

---

## Contenido
El repositorio principal se divide en los siguientes componentes fundamentales:
- `/firmware/brazalete/` -> Código fuente (Arduino IDE o ESP-IDF) para el wearable (ESP32-C3 Super Mini). Ver su README.
- `/firmware/semaforo/` -> Código fuente (Arduino IDE o ESP-IDF) para la baliza de infraestructura (ESP32 DevKit V1). Ver su README.
- `/ACPIGPS/` -> Aplicación móvil Android nativa en Kotlin (Jetpack Compose).
- `/docs/` -> Documentación adicional y guías de configuración de la nube.
- `Justfile` -> Orquestador de tareas para compilación y *linting* automatizado.

---

## Explicación
### Arquitectura y Flujo de Procesamiento

El flujo de información se ejecuta de la siguiente forma:

```text
[Semáforo ESP32]  ──ESP-NOW──►  [Brazalete ESP32-C3]  ──BLE──►  [App Android]  ──HTTPS──►  [Grafana Cloud]
 QMC6308                         QMC6308 + MPU6050              GPS + Pasos
 2 LEDs simulan                  Brújula + Decisión              Registro sesiones
 semáforo                        Feedback háptico                Telemetría
 (sin WiFi/Internet)             FSM de 2Hz
```

1. **Semáforo (Infraestructura)**: Un ESP32 DevKit V1 equipado con un magnetómetro lee su orientación magnética estática y simula un ciclo de semáforo verde/rojo. Transmite su orientación (heading) y estado por ESP-NOW a 2Hz (cada 500ms).
2. **Brazalete (Wearable)**: Actúa como el centro de cómputo en el borde. Lee su orientación tilt-compensada (acelerómetro + magnetómetro), y calcula la diferencia angular frente a la infraestructura. Si detecta que el usuario está correctamente enfrentado (`|Δθ - 180°| < 30°`), provee feedback háptico diferenciado mediante dos micromotores de vibración.
3. **App Android**: Recibe mediante BLE los paquetes de telemetría del brazalete, controla geocercas y envía métricas asíncronas.
4. **Grafana Cloud**: Ingesta los datos del usuario asíncronamente para generar mapas y analíticas en tiempo real.

---

## Dependencias
- **Hardware**: 
  - ESP32-C3 Super Mini, ESP32 DevKit V1.
  - MPU6050 (Acelerómetro/Giroscopio) y QMC6308 (Magnetómetro I2C).
  - Micro-motores de vibración 3V tipo moneda.
- **Software**: 
  - [Arduino IDE](https://www.arduino.cc/en/software) con el paquete *esp32 by Espressif Systems* 3.x (o ESP-IDF 5.5) para compilar y flashear el firmware.
  - [Just](https://github.com/casey/just) como *command runner* (opcional pero recomendado).
  - Android Studio para compilar la App.

---

## Ejemplo de Uso

Los dos firmwares son sketches de Arduino IDE (también compilan con ESP-IDF). Abrir en
Arduino IDE `firmware/brazalete/brazalete.ino` o `firmware/semaforo/semaforo.ino` y
cargarlos con la configuración de placa que indica el README de cada uno.

Desde la terminal con `arduino-cli`:
```bash
# Brazalete (ESP32-C3 Super Mini, Serial por el USB nativo)
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc firmware/brazalete

# Semáforo (ESP32 DevKit V1)
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/semaforo
```

Las pruebas de la lógica del brazalete corren en la PC, sin placa (ver
`firmware/brazalete/test/README.md`):
```bash
sh firmware/brazalete/test/ejecutar_pruebas.sh
```
