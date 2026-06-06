# 🚦 ACPI: Asistente de Cruce Peatonal para Invidentes v2.0 👨‍🦯➡️

**ACPI** es un sistema **IoT de asistencia a la movilidad** diseñado para brindar **autonomía y seguridad** a personas con discapacidad visual al cruzar calles en intersecciones semaforizadas.

El sistema combina **hardware en el borde (Edge Computing)** con sensores locales en la pulsera y el semáforo, comunicación local ultrarrápida y una **aplicación móvil de telemetría** integrada con **Grafana Cloud** para el monitoreo pasivo de la seguridad de la sesión.

---

## 🏗️ Arquitectura de ACPI v2.0

El flujo de información y procesamiento se realiza de la siguiente manera:

```
[Semáforo ESP32]  ──ESP-NOW──►  [Pulsera ESP32-C3]  ──BLE──►  [App Android]  ──HTTPS──►  [Grafana Cloud]
 QMC5883L                        QMC5883L + MPU6050              GPS + Pasos
 2 LEDs simulan                  Brújula + Decisión              Registro sesiones
 semáforo                        Feedback háptico                Telemetría
 (sin WiFi/Internet)
```

1. **Semáforo (ESP32 DevKit V1)**: Detecta su orientación (grados) y simula el estado físico del semáforo alternando dos LEDs (Verde/Rojo) de forma automatizada mediante una FSM temporizada. Transmite su orientación y estado mediante ESP-NOW.
2. **Pulsera (ESP32-C3 Super Mini)**: Hub central de decisiones. Lee su orientación tilt-compensada (MPU6050 + QMC5883L), determina si el usuario está alineado correctamente con la vía, y da retroalimentación háptica (vibración) acorde. Reporta datos de la sesión cada 500ms al móvil por BLE.
3. **App Android**: Recibe datos de estado de la pulsera, los muestra en UI, rastrea los pasos y geocercas locales (alertando por notificaciones nativas Android), y reporta la telemetría agregada de la sesión al finalizar el cruce.
4. **Grafana Cloud**: Ingesta la telemetría aplanada enviada de forma asíncrona y resiliente por la App para graficar mapas de cruces, estadísticas de pánico y tiempos de espera.

---

## ✨ Novedades de la Versión 2.0
- **Lógica en el Borde (Edge Computing)**: El cálculo de orientación y control de motores se realiza localmente en la pulsera, eliminando latencias y dependencias de procesamiento en el móvil.
- **FSM en Pulsera**: Máquina de estados formal (`BOOTING`, `STANDBY_LOW_POWER`, `SENSING_CROSSING`, `ALERT_PANIC`) con soporte de **suspensión Light Sleep** en standby para maximizar la autonomía de la batería.
- **Botón de Pánico por Hardware**: Trigger inmediato y prioritario mediante interrupciones físicas (ISR) usando el botón BOOT integrado.
- **Sin Dependencias Firebase ni Telegram**: Reemplazados por notificaciones locales nativas de Android y envío directo a Grafana Cloud, reduciendo el consumo de batería y datos móviles.
- **Resiliencia en Telemetría**: Ring-buffer en memoria en la app Android para encolar métricas y reintentar el envío cuando regrese la conexión.

---

## ⚙️ Requisitos de Hardware
- 1 × **ESP32 DevKit V1** (Semáforo)
- 1 × **ESP32-C3 Super Mini** (Pulsera)
- 1 × **Magnetómetro QMC5883L** (Semáforo)
- 1 × **Giroscopio/Acelerómetro MPU6050** (Pulsera)
- 1 × **Magnetómetro QMC5883L** (Pulsera)
- 2 × **Motores de vibración** (Pulsera)
- 2 × **LEDs de simulación** (Rojo/Verde para el Semáforo)
- **Smartphone Android** con Bluetooth LE, GPS y sensor de pasos.

---

## 📂 Estructura del Código

- [/firmware/pulsera/](file:///c:/Users/sntav/programacion/ACPI/firmware/pulsera/) -> Firmware para el ESP32-C3 Super Mini de la pulsera.
- [/firmware/semaforo/](file:///c:/Users/sntav/programacion/ACPI/firmware/semaforo/) -> Firmware para el ESP32 DevKit V1 del semáforo.
- [/ACPIGPS/](file:///c:/Users/sntav/programacion/ACPI/ACPIGPS/) -> Aplicación móvil nativa en Kotlin/Jetpack Compose.
- [/docs/](file:///c:/Users/sntav/programacion/ACPI/docs/) -> Guías de configuración de Grafana Cloud y especificación del protocolo BLE.

---

## 🧑‍💻 Autores
Proyecto desarrollado por:  
**Dilan Osorio, Andrea Cárdenas y Nicolás Rodríguez**  
📚 Asesoría: *Johanna Carolina Sánchez Ramírez*
