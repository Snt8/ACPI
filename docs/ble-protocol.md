# Protocolo de Comunicación BLE (Pulsera ◄► App)

Este documento detalla el protocolo de comunicación inalámbrica vía **Bluetooth Low Energy (BLE)** utilizado entre la **Pulsera Inteligente** (Servidor GATT) y la **Aplicación Android** (Cliente GATT).

---

## 1. Identificación y Servicios GATT

La pulsera se anuncia con el nombre del dispositivo **`ACPI_Pulsera`**.

### UUIDs del Perfil GATT:
- **Servicio Principal**: `12345678-1234-1234-1234-123456789abc`
- **Característica de Transmisión (TX - Notificaciones)**: `87654321-4321-4321-4321-cba987654321`
  * *Permiso*: Notificación (Notify). La pulsera empuja datos aquí periódicamente.
- **Característica de Recepción (RX - Escritura)**: `11111111-2222-3333-4444-555555555555`
  * *Permiso*: Escritura sin respuesta (Write Without Response / Write). La app escribe comandos aquí.

---

## 2. Payload de Telemetría (Pulsera ──► App)

La pulsera transmite cada **500 ms** (cuando hay conexión y está en estado activo de cruce o pánico) un JSON compacto en texto plano UTF-8 a través de la característica **TX**:

```json
{"hd":135,"st":1,"ok":true,"tr":18,"bt":-1}
```

### Especificación de Campos:

| Clave | Tipo | Rango | Descripción |
|---|---|---|---|
| **`hd`** | Entero | `0` a `359` | **Heading**: Orientación magnética tilt-compensada actual de la pulsera. |
| **`st`** | Entero | `0` a `3` | **Status**: Estado actual del cruce determinado por la pulsera:<br>• `0`: Espera (Semáforo en verde de autos, peatón debe esperar).<br>• `1`: Cruce Permitido (Semáforo en rojo de autos, peatón alineado).<br>• `2`: Orientación Incorrecta (El peatón no está mirando en la dirección del semáforo).<br>• `3`: Alerta de Pánico (Botón de pánico presionado por el usuario). |
| **`ok`** | Booleano | `true` / `false` | **Alineación**: `true` si el heading de la pulsera coincide con el heading del semáforo dentro del umbral (±45°). |
| **`tr`** | Entero | `0` a `255` | **Tiempo Restante**: Segundos restantes en el estado actual del semáforo. |
| **`bt`** | Entero | `-1` o `0` a `100` | **Batería**: Nivel de batería en porcentaje. Si el hardware no posee divisor resistivo en ADC para medir batería, se reportará **`-1`** (no disponible). |

---

## 3. Comandos de Control (App ──► Pulsera)

La aplicación móvil puede enviar comandos a través de la característica **RX** para propósitos de prueba y depuración:

- **`TEST_ALERT`**: Genera un pulso haptico de prueba de corta duración en ambos motores de la pulsera para comprobar la conectividad del vibrador.
- **`VIBRATE_STOP`**: Detiene cualquier vibración activa inmediatamente.
