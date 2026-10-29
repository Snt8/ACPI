# Protocolo de Comunicación BLE (Brazalete ◄► App)

Este documento detalla el protocolo de comunicación inalámbrica vía **Bluetooth Low Energy (BLE)** entre el **Brazalete** (servidor GATT) y la **app móvil** (`app-movil/`, cliente GATT).

| Lado | Código |
|---|---|
| Brazalete | `firmware/brazalete/src/comunicacion/movil/` (`movil.cpp`, `paquete_telemetria.cpp`) y `src/control/fsm.cpp` (`enviarTelemetria`) |
| App | `app-movil/src/protocolo/brazalete.ts` (parser) y `src/ble/cliente-ble.ts` |

Si se cambia un campo, hay que cambiarlo en los dos lados y en este documento. El ejemplo de la sección 2 está copiado tal cual en las pruebas de ambos (`firmware/brazalete/test/test_telemetria.cpp` y `app-movil/src/protocolo/__tests__/brazalete.test.ts`).

---

## 1. Identificación y Servicios GATT

El brazalete se anuncia con el nombre **`ACPI_Brazalete`** y sólo enciende el BLE cerca de un semáforo: en standby (30 s sin paquetes del semáforo) el BLE está apagado. La app re-escanea y reconecta sola.

- **Servicio**: `12345678-1234-1234-1234-123456789abc` (va en el *advertising*: iOS sólo escanea en segundo plano filtrando por servicio).
- **TX (Notify)**: `87654321-4321-4321-4321-cba987654321` — telemetría, brazalete → app.
- **RX (Write)**: `11111111-2222-3333-4444-555555555555` — comandos, app → brazalete.
- **MTU**: el brazalete acepta hasta **185** (`MTU_BLE` en `movil.cpp`). El paquete mide hasta 160 bytes y una notificación lleva MTU − 3: con el MTU por defecto (23) llegaría cortado. Android lo pide al conectar (`requestMTU(185)` en `cliente-ble.ts`); iOS lo negocia solo.

---

## 2. Telemetría (Brazalete ──► App), versión 2

Cada **500 ms**, mientras hay un semáforo a la vista y un móvil conectado, el brazalete notifica un JSON compacto en ASCII por **TX**:

```json
{"v":2,"n":412,"hd":21,"st":1,"ok":true,"tr":18,"bt":-1,"id":1,"lz":1,"hs":200,"gr":-1,"br":1,"pt":4,"rx":20,"rs":-67}
```

### Campos de la versión 1 (sin cambios)

| Clave | Tipo | Rango | Descripción |
|---|---|---|---|
| **`hd`** | Entero | `0`-`359` | Rumbo magnético del brazalete, con calibración y compensación de inclinación. |
| **`st`** | Entero | `0`-`3` | Estado del cruce:<br>• `0`: de frente al cruce, paso peatonal **en rojo**.<br>• `1`: de frente al cruce, paso peatonal **en verde**.<br>• `2`: mal orientada (no está de frente; el brazalete vibra hacia dónde girar).<br>• `3`: pánico (reservado: el brazalete todavía no lo envía). |
| **`ok`** | Booleano | | `true` si está de frente al cruce: desviación menor a **5°** (confirmada con 2 lecturas seguidas). Se pierde al pasar de 10°, o de 5° si la luz todavía no se avisó (ver `firmware/brazalete/patrones.md`). |
| **`tr`** | Entero | `0`-`255` | Segundos que le quedan a la luz actual. |
| **`bt`** | Entero | `-1` o `0`-`100` | Batería en %. La PCB v3.5 no la mide: siempre **`-1`**. |

### Campos nuevos de la versión 2

| Clave | Tipo | Rango | Descripción | Para qué la usa la app |
|---|---|---|---|---|
| **`v`** | Entero | `2` | Versión del paquete. Sin `v`, es un firmware de la versión 1. | Saber si vienen los campos de abajo. |
| **`n`** | Entero | `0`-`65535` | Secuencia: sube 1 por paquete y vuelve a 0 tras 65535. | Contar notificaciones BLE perdidas (saltos). |
| **`id`** | Entero | `0`-`255` | `id` del semáforo que sigue el brazalete (el primero que escuchó al salir de standby). | Unir la sesión con el semáforo. |
| **`lz`** | Entero | `0`/`1` | Luz peatonal: `1` verde, `0` rojo. A diferencia de `st`, llega **aunque esté mal orientada**. | Diagnóstico y métricas de espera. |
| **`hs`** | Entero | `0`-`359` | Rumbo que envía el semáforo. De frente al cruce es mirar hacia `hs + 180`. | Diagnóstico (error de orientación). |
| **`gr`** | Entero | `-180`-`180` | Giro para quedar de frente: positivo = derecha, negativo = izquierda. | Diagnóstico; a futuro, aviso por voz detallado. |
| **`br`** | Entero | `0`-`2` | Brújula: `0` **sin calibrar** (el brazalete no indica giros ni la luz hasta calibrarla), `1` lectura válida, `2` lectura descartada (muñeca en movimiento, giro brusco o sensor). | Avisar por voz que falta calibrar. |
| **`pt`** | Entero | `0`-`5` | Patrón que vibra en ese momento: `0` ninguno, `1` semáforo detectado, `2` girar a la derecha, `3` girar a la izquierda, `4` verde, `5` rojo. | Diagnóstico. |
| **`rx`** | Entero | `0`-`32` | Paquetes del semáforo seguido recibidos en los últimos 10 s. El semáforo manda 2 por segundo: **20 = sin pérdidas**. | Métrica de paquetes ESP-NOW perdidos. |
| **`rs`** | Entero | `-127`-`0` | Señal (RSSI, dBm) del último paquete del semáforo. `0` = sin dato. | Métrica de alcance. |

La app es tolerante: un JSON cortado o sin `st` se descarta sin cortar la conexión, y un campo de la versión 2 que falte o venga fuera de rango queda como desconocido (`null`) sin descartar el paquete.

---

## 3. Comandos (App ──► Brazalete)

Texto ASCII exacto, sin `\0` ni salto de línea, por **RX**:

- **`TEST_ALERT`**: vibración de prueba con el patrón de "semáforo detectado" (3 pulsos cortos, ambos motores). Respeta las reglas de las soldaduras: si sonó otro patrón hace menos de 3 s (o terminó hace menos de 1 s), se ignora y el monitor serie lo dice.
- **`VIBRATE_STOP`**: corta el patrón que esté sonando. Cortar no adelanta los 3 s hasta el siguiente.

El brazalete atiende como mucho un comando cada 200 ms y sólo con un semáforo a la vista (fuera de eso el BLE está apagado).
