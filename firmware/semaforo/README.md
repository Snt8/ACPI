# Firmware Semáforo (Infraestructura ACPI)

## Descripción General
Este es el código fuente principal del **Nodo Esclavo (Semáforo)** del proyecto ACPI. Está diseñado para ejecutarse en una placa de desarrollo **ESP32 DevKit V1** (o compatible) anclada de manera estática y fija en la intersección peatonal.

Su misión principal es leer el campo magnético direccional permanente hacia donde cruza la calle (su propia orientación), simular visualmente mediante LEDs el estado de permiso de vía (Verde/Rojo para vehículos), y disparar de forma constante la información (telemetría) a las pulseras cercanas usando difusión de radio (Broadcast).

---

## Contenido
- `platformio.ini` -> Archivo de configuración de construcción de PlatformIO (Framework `espressif32@6.9.0`).
- `include/` -> Constantes globales (Ej: Tiempos de duración de la luz verde o roja) y configuración (`config.h`, `const.h`).
- `src/main.cpp` -> Punto de entrada y Máquina de Estados Finita (FSM) basada en tiempo del semáforo.
- `lib/` -> Submódulos de hardware y red (`brujula`, `comunicacion`, `protocolo`).

---

## Explicación
A diferencia de la pulsera que es impulsada por eventos, el semáforo está impulsado por reloj (Temporizadores):

1. Mantiene un bucle continuo de `millis()` verificando si la duración preconfigurada del estado actual del semáforo (ej: 30 segundos) ha expirado. Si expira, alterna el relé/LED y cambia internamente su estado booleano de `permitidoCruce`.
2. A una tasa paralela e independiente, lee periódicamente (ej: 2 Hz) el magnetómetro I2C para obtener su propia dirección en grados frente al polo magnético terrestre.
3. Empaqueta el tiempo que le resta a la luz actual, los grados y la validación en una estructura C empaquetada.
4. Invoca la radio ESP-NOW, propagando la carga en modo BROADCAST (Mac: `FF:FF:FF:FF:FF:FF`). Toda pulsera en rango capturará esta ráfaga de forma pasiva.

---

## Dependencias
- Librería base **Arduino.h**.
- Bus I2C de Espressif (`<Wire.h>`).

---

## Ejemplo de Uso

Para compilar y cargar localmente:

```bash
# Cambiar al directorio raíz
cd ../../

# Ejecutar linters de calidad
pio check -d firmware/semaforo

# Compilar para la placa DevKit
pio run -d firmware/semaforo

# Flashear al hardware conectado por USB
pio run -d firmware/semaforo --target upload
```
