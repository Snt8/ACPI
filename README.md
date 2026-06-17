# 🚦 ACPI: Asistente de Cruce Peatonal para Invidentes v2.0 👨‍🦯➡️

## Descripción General
**ACPI** es un sistema **IoT de asistencia a la movilidad** diseñado para brindar **autonomía y seguridad** a personas con discapacidad visual al cruzar calles en intersecciones semaforizadas.

El sistema combina **hardware en el borde (Edge Computing)** con sensores magnéticos e inerciales locales en una pulsera (wearable) y en un semáforo (infraestructura). Utiliza comunicación local ultrarrápida (ESP-NOW) entre nodos y telemetría por Bluetooth Low Energy (BLE) hacia una aplicación móvil integrada con **Grafana Cloud** para monitorear la seguridad de la sesión.

---

## Contenido
El repositorio principal se divide en los siguientes componentes fundamentales:
- `/firmware/pulsera/` -> Código fuente (PlatformIO/C++) para el wearable (ESP32-C3 Super Mini).
- `/firmware/semaforo/` -> Código fuente (PlatformIO/C++) para la baliza de infraestructura (ESP32 DevKit V1).
- `/ACPIGPS/` -> Aplicación móvil Android nativa en Kotlin (Jetpack Compose).
- `/docs/` -> Documentación adicional y guías de configuración de la nube.
- `Justfile` -> Orquestador de tareas para compilación y *linting* automatizado.

---

## Explicación
### Arquitectura y Flujo de Procesamiento

El flujo de información se ejecuta de la siguiente forma:

```text
[Semáforo ESP32]  ──ESP-NOW──►  [Pulsera ESP32-C3]  ──BLE──►  [App Android]  ──HTTPS──►  [Grafana Cloud]
 QMC6308                         QMC6308 + MPU6050              GPS + Pasos
 2 LEDs simulan                  Brújula + Decisión              Registro sesiones
 semáforo                        Feedback háptico                Telemetría
 (sin WiFi/Internet)             FSM de 2Hz
```

1. **Semáforo (Infraestructura)**: Un ESP32 DevKit V1 equipado con un magnetómetro lee su orientación magnética estática y simula un ciclo de semáforo verde/rojo. Transmite su orientación (heading) y estado por ESP-NOW a 2Hz (cada 500ms).
2. **Pulsera (Wearable)**: Actúa como el centro de cómputo en el borde. Lee su orientación tilt-compensada (acelerómetro + magnetómetro), y calcula la diferencia angular frente a la infraestructura. Si detecta que el usuario está correctamente enfrentado (`|Δθ - 180°| < 30°`), provee feedback háptico diferenciado mediante dos micromotores de vibración.
3. **App Android**: Recibe mediante BLE los paquetes de telemetría de la pulsera, controla geocercas y envía métricas asíncronas.
4. **Grafana Cloud**: Ingesta los datos del usuario asíncronamente para generar mapas y analíticas en tiempo real.

---

## Dependencias
- **Hardware**: 
  - ESP32-C3 Super Mini, ESP32 DevKit V1.
  - MPU6050 (Acelerómetro/Giroscopio) y QMC6308 (Magnetómetro I2C).
  - Micro-motores de vibración 3V tipo moneda.
- **Software**: 
  - [PlatformIO](https://platformio.org/) para el desarrollo y flasheo C++ del firmware.
  - [Just](https://github.com/casey/just) como *command runner* (opcional pero recomendado).
  - Android Studio para compilar la App.

---

## Ejemplo de Uso

Para compilar e inyectar el código en los microcontroladores usando PlatformIO, primero asegúrate de tener instaladas las dependencias.

Con **Just** (Recomendado):
```bash
# Compilar todo el ecosistema firmware
just compile-all

# Flashear el semáforo y la pulsera (asegúrate de conectarlos vía USB)
just flash-semaforo
just flash-pulsera
```

Sin **Just** (Usando PlatformIO CLI directo):
```bash
pio run -d firmware/pulsera --target upload
pio run -d firmware/semaforo --target upload
```
