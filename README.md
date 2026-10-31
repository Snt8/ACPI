# 🚦 ACPI: Asistente de Cruce Peatonal para Invidentes v3.5 👨‍🦯➡️

## Descripción General
**ACPI** es un sistema **IoT de asistencia a la movilidad** que da **autonomía y seguridad** a personas con discapacidad visual al cruzar calles con semáforo.

Un **semáforo** (baliza fija en la esquina) transmite su orientación y la luz peatonal por ESP-NOW. Un **brazalete** lee la orientación de la persona con su propia brújula, la compara con la del cruce y le indica con vibraciones hacia dónde girar y cuándo la luz está en verde o en rojo. Por Bluetooth Low Energy (BLE) el brazalete envía su telemetría a una **app móvil** accesible (iOS y Android), que avisa por voz y registra los cruces en la **API de ACPI**. Las cifras anónimas se publican en **[ACPI Web](https://snt8.github.io/acpi-website/)**.

Esta es la versión 3.5: PCB propia para el brazalete, firmware reescrito para Arduino IDE / ESP-IDF y app en React Native. La versión 2, que ganó la **medalla de oro en Infomatrix Sudamérica 2025**, está en el [repositorio original](https://github.com/Dilaansg/ACPI).

---

## Contenido
- `firmware/brazalete/` -> Firmware del brazalete (ESP32-C3 Super Mini): sketch de Arduino IDE que también compila con ESP-IDF, con pruebas en la PC (`test/`) y sketches de prueba del hardware (`pruebas/`). Los patrones de vibración están en `patrones.md`. Ver su README.
- `firmware/semaforo/` -> Firmware del semáforo (ESP32 DevKit V1), con la misma estructura. Ver su README.
- `app-movil/` -> App móvil para iOS y Android (React Native + Expo), pensada para usarse con VoiceOver y TalkBack. Ver su README.
- `docs/` -> Protocolos y diseño:
  - `ble-protocol.md`: contrato BLE entre el brazalete y la app.
  - `servidor-api.md`: contrato HTTPS entre la app y la API.
  - `guia-app-invidentes.md`: guía de diseño accesible de la app.
  - `figuras/`: figura de arquitectura, que también usa el sitio web.
- `electronics/` -> PCB del brazalete en EasyEDA: `acpiv3.5-design` (la placa v3.5, la que está en uso) y `acpiv3-new-design`. Cada carpeta trae esquemático, BOM y Gerbers.
- `3D-models/` -> Carcasa del brazalete. Los STL se generan con `generar_case.py`; para cambiar la carcasa se cambian sus parámetros, no las mallas.
- `assets/` -> Logos de ACPI (el vigente es `ACPI-V3.5-Logo-updated.pdf`).

El sitio web y la API están en otro repositorio: **[Snt8/acpi-website](https://github.com/Snt8/acpi-website)**.

---

## Explicación
### Arquitectura y Flujo de Procesamiento

```text
[Semáforo ESP32] ─ESP-NOW─► [Brazalete ESP32-C3] ─BLE─► [App móvil] ─HTTPS─► [ACPI API] ──► [ACPI Web]
 QMC6308                     QMC5883L + MPU6500         iOS / Android        FastAPI +       GitHub Pages
 2 LEDs simulan la luz       Brújula a 5 Hz             Voz y lector          Supabase en
 25 s por luz                2 motores de vibración     de pantalla, GPS      Render
 Paquete a 2 Hz              Standby sin semáforo       Semáforos guardados   (acpi-website/api)
```

1. **Semáforo (infraestructura)**: un ESP32 DevKit V1 con un magnetómetro QMC6308 mide su orientación, ya calibrada y promediada. Simula con dos LEDs un ciclo de 25 s por luz. Cada 500 ms transmite por ESP-NOW (broadcast) un paquete de 10 bytes con su rumbo, la luz y el tiempo restante; el paquete lleva la firma `"ACPI"` y un CRC-8.
2. **Brazalete (wearable)**: un ESP32-C3 Super Mini en la PCB v3.5, con un GY-271 (QMC5883L) y un MPU6500. Calcula a 5 Hz su rumbo con calibración *hard-iron* y compensación de inclinación. La persona queda de frente al cruce cuando `|Δθ − 180°| < 5°` en dos lecturas seguidas.
   - Con dos micromotores indica el lado hacia el que hay que girar y luego avisa la luz peatonal: verde con dos pulsos largos, rojo con uno. Cada evento vibra una sola vez.
   - Tras 30 s sin paquetes del semáforo pasa a **standby**: apaga sensores, motores y BLE, y escucha 1.2 s cada 5 s.
3. **App móvil (iOS y Android)**: se conecta al brazalete por BLE. El brazalete sólo enciende el BLE cerca de un semáforo, así que la app lo busca y se reconecta sola.
   - Describe el estado del cruce por voz o con el lector de pantalla, sin ordenar nunca cruzar.
   - Guarda los semáforos del usuario en el teléfono.
   - Mide cada cruce con el GPS (espera, duración, paquetes perdidos, señal) y lo envía a la API. Si no hay red, lo guarda en una cola local.
4. **ACPI API y ACPI Web** ([acpi-website](https://github.com/Snt8/acpi-website)): la API (FastAPI + Supabase, que se despliega en Render con `render.yaml`) recibe los eventos `cruce` y `semaforo` según el contrato de `docs/servidor-api.md`. El sitio en GitHub Pages presenta el proyecto y publica el panel de telemetría con datos anónimos.

---

## Dependencias
- **Hardware**:
  - Brazalete: ESP32-C3 Super Mini sobre la PCB v3.5, MPU6500 (I2C 0x68), GY-271 con QMC5883L (I2C 0x0D), 2 micromotores de vibración tipo moneda y batería LiPo con conector JST-PH.
  - Semáforo: ESP32 DevKit V1, magnetómetro QMC6308 (I2C 0x2C) y 2 LEDs.
- **Software**:
  - [Arduino IDE](https://www.arduino.cc/en/software) con el paquete *esp32 by Espressif Systems* 3.x (probado con 3.3.9), o ESP-IDF 5.5.
  - g++ o clang++ para las pruebas del brazalete en la PC (en Windows, el de WSL).
  - Node 20 o superior, y una Mac con Xcode (iOS) o Android Studio (Android) para compilar la app. La app usa módulos nativos (Bluetooth, MapLibre), así que necesita un *development build*; no funciona en Expo Go.
  - Python 3 con `numpy`, `trimesh` y `manifold3d` para regenerar la carcasa.

---

## Ejemplo de Uso

Los dos firmwares son sketches de Arduino IDE (también compilan con ESP-IDF). Hay que abrir
`firmware/brazalete/brazalete.ino` o `firmware/semaforo/semaforo.ino` en Arduino IDE y
cargarlos con la placa que indica el README de cada uno.

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

La app se compila e instala en el teléfono con un *development build*. Para iOS, desde una
Mac, con `npx expo run:ios --device` (los pasos están en `app-movil/README.md`):
```bash
cd app-movil
npm install
npx expo run:android
```

Para probar la app con el brazalete real, el semáforo tiene que estar encendido cerca: el
brazalete no enciende el Bluetooth hasta recibir su primer paquete.

---

## Autores
- **Edy S. Avila** ([@Snt8](https://github.com/Snt8))
- **Dilan Osorio** ([@Dilaansg](https://github.com/Dilaansg))
- **Andrea Cárdenas** ([@AndreaCarod](https://github.com/AndreaCarod))
- **Nicolás Rodríguez** ([@NicolasRodriguez08](https://github.com/NicolasRodriguez08))

Dilan, Andrea y Nicolás formaron el equipo de ACPI v2, en el [repositorio original](https://github.com/Dilaansg/ACPI).
