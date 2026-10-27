# Firmware Semáforo (Infraestructura ACPI)

## Descripción General
Este es el código fuente principal del **Nodo Esclavo (Semáforo)** del proyecto ACPI. Está diseñado para ejecutarse en una placa de desarrollo **ESP32 DevKit V1** (o compatible) anclada de manera estática y fija en la intersección peatonal.

Su misión principal es leer el campo magnético direccional permanente hacia donde cruza la calle (su propia orientación), simular visualmente mediante LEDs el estado de permiso de vía (Verde/Rojo para vehículos), y disparar de forma constante la información (telemetría) a los brazaletes cercanos usando difusión de radio (Broadcast).

Se compila con **Arduino IDE** o con **ESP-IDF**, con la misma estructura que el firmware del brazalete; ya no necesita PlatformIO.

---

## Contenido
- `semaforo.ino` -> Punto de entrada y Máquina de Estados Finita (FSM) basada en tiempo del semáforo. Es el archivo que se abre en Arduino IDE.
- `src/` -> Todo el resto del código; Arduino IDE lo compila solo.
  - `config.h` -> Pines, tiempos de cada luz (25 s), intervalo de envío ESP-NOW y constantes de calibración.
  - `const.h` -> Constantes matemáticas.
  - `brujula/`, `comunicacion/`, `protocolo/` -> Submódulos de hardware y red.
- `main/`, `CMakeLists.txt`, `sdkconfig.defaults` -> Archivos de ESP-IDF (Arduino como componente). Arduino IDE los ignora.

---

## Explicación
A diferencia del brazalete que es impulsado por eventos, el semáforo está impulsado por reloj (Temporizadores):

1. Mantiene un bucle continuo de `millis()` verificando si la duración preconfigurada del estado actual del semáforo (25 segundos por luz, `DURACION_VERDE_MS` / `DURACION_ROJO_MS`) ha expirado. Si expira, alterna el relé/LED y cambia internamente su estado booleano de `permitidoCruce`.
2. A una tasa paralela e independiente, lee periódicamente (ej: 2 Hz) el magnetómetro I2C para obtener su propia dirección en grados frente al polo magnético terrestre.
3. Empaqueta el tiempo que le resta a la luz actual, los grados y la validación en una estructura C empaquetada.
4. Invoca la radio ESP-NOW, propagando la carga en modo BROADCAST (Mac: `FF:FF:FF:FF:FF:FF`). Todo brazalete en rango capturará esta ráfaga de forma pasiva.

---

## Dependencias
- Librería base **Arduino.h**.
- Bus I2C de Espressif (`<Wire.h>`).

---

## Compilar con Arduino IDE
1. **Herramientas → Placa → Gestor de placas**: instalar *esp32 by Espressif Systems* (3.x).
2. Abrir `firmware/semaforo/semaforo.ino`.
3. En **Herramientas → Placa** elegir **ESP32 Dev Module** (el DevKit V1).
4. Subir y abrir el monitor serie a **115200** baudios.

Lo mismo desde la terminal con `arduino-cli`:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/semaforo
```

## Compilar con ESP-IDF
Requiere **ESP-IDF v5.5** (la versión sobre la que está construido Arduino-ESP32 3.3). El componente `espressif/arduino-esp32` se descarga solo la primera vez (`main/idf_component.yml`).
```bash
cd firmware/semaforo
idf.py set-target esp32
idf.py build flash monitor
```
`main/main.cpp` sólo incluye `semaforo.ino`, así que ambos entornos compilan exactamente el mismo código.

> El paquete ESP-NOW lleva la firma `"ACPI"` y un CRC-8 (10 bytes). Semáforo y brazalete
> tienen que flashearse con versiones del mismo protocolo: el brazalete descarta cualquier
> paquete con otro tamaño, otra firma o el CRC equivocado.
