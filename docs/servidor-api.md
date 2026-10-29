# API del servidor web de ACPI (Render)

## Descripción General
Contrato entre la app móvil (`app-movil/`) y el servidor web de ACPI alojado en
[Render](https://render.com). La app manda cada cruce y cada cambio en los semáforos
guardados. El servidor los almacena y la web de ACPI los muestra (por ejemplo, en un mapa).

```text
[Brazalete] ──BLE──► [App móvil] ──HTTPS──► [Servidor en Render] ──► [Base de datos] ──► [Web de ACPI]
                     GPS + cola SQLite
                     (espera si no hay red)
```

El servidor puede estar hecho con cualquier tecnología (Node/Express, Python/FastAPI, etc.).
Sólo tiene que cumplir lo de este documento.

---

## Contenido
- Configuración en la app
- `GET /api/salud`
- `POST /api/eventos`
- Formato de los eventos
- Recomendaciones para el servidor

---

## Explicación

### Configuración en la app
En `app-movil/.env` (copiar de `.env.example`; no se sube a git):

```bash
EXPO_PUBLIC_API_URL=https://acpi.onrender.com
EXPO_PUBLIC_API_TOKEN=un-token-largo-al-azar
```

- `EXPO_PUBLIC_API_URL`: la URL que Render le da al servicio, sin barra final. Sin ella la app
  guarda los eventos en la cola y no envía nada.
- `EXPO_PUBLIC_API_TOKEN`: opcional. Si existe, la app lo manda como `Authorization: Bearer <token>`.
- Expo copia estas variables dentro de la app, así que el token **no es secreto**: sólo debe
  permitir escribir eventos, nunca leer ni borrar datos.
- El `.env` tiene que estar en la computadora que corre `npm start` o compila la app.

Para probar: en la app, **Ajustes → Diagnóstico → Enviar dato de prueba**.

### `GET /api/salud`
Responde `200` con cualquier cuerpo. La app lo llama sin esperar respuesta en cuanto el
brazalete detecta un semáforo. Así el servidor ya está despierto cuando el cruce termina y llega
el evento. En el plan gratis, Render duerme el servicio tras unos minutos sin uso y la primera
petición tarda en despertarlo.

### `POST /api/eventos`
Recibe un lote de hasta 100 eventos.

```http
POST /api/eventos
Content-Type: application/json
Authorization: Bearer <EXPO_PUBLIC_API_TOKEN>

{ "eventos": [ { "v": 3, "id": "mh3k2b0q8d2e", "tipo": "cruce", ... }, ... ] }
```

Qué hace la app con cada respuesta:

| Respuesta | La app |
|---|---|
| `2xx` (200, 201, 204) | Borra el lote de su cola. |
| `400`, `413`, `422` | Descarta el lote: uno mal formado nunca se va a aceptar. |
| Cualquier otra (`401`, `403`, `404`, `429`, `5xx`), o sin respuesta en 90 s | Lo deja en la cola y lo reintenta: al minuto, al volver a abrir la app o al llegar otro evento. |

**Duplicados:** si la respuesta se pierde, la app reenvía el mismo lote. Cada evento trae un
`id` único: el servidor debe ignorar los que ya tenga (por ejemplo, con `UNIQUE (id)` y
`ON CONFLICT DO NOTHING`).

La cola del teléfono guarda hasta 1000 eventos; si se llena, se pierden los más viejos. Si el
usuario desactiva **Ajustes → Compartir cruces con ACPI**, la app no encola nada y borra lo
pendiente.

### Formato de los eventos
Todos los eventos llevan:

| Campo | Qué es |
|---|---|
| `v` | Versión del formato (3; en la 2 el `cruce` no traía las métricas de enlaces). |
| `id` | Identificador único del evento, para descartar duplicados. |
| `tipo` | `cruce`, `semaforo` o `prueba`. |
| `dispositivo` | Seudónimo aleatorio del teléfono. No identifica a la persona. |
| `momento` | Cuándo se generó, en ms desde epoch (hora del teléfono). |

**Guardar el JSON completo de cada evento**, aunque el servidor todavía no use todos los campos.
La app puede agregar campos nuevos sin cambiar `v`, por ejemplo las métricas del enlace BLE.

#### `cruce`
Se envía al terminar cada sesión. Una sesión va desde que el brazalete encuentra un semáforo y
se conecta al teléfono hasta que se desconecta, 30 s después de perderlo.

```json
{
  "v": 3, "id": "mh3k2b0q8d2e", "tipo": "cruce", "dispositivo": "mh3k2a9x1c4f",
  "momento": 1793300000000, "sesion": "mh3k2b0q7a1b", "inicio": 1793299940000,
  "simulado": false, "lat": 4.60971, "lon": -74.08175, "precision_m": 6, "semaforo_id": 3,
  "espera_s": 21.4, "cruzo": true, "duracion_cruce_s": 13.2, "verde_restante_s": 25,
  "rumbo_cruce": 92, "veces_mal_orientado": 1, "duracion_sesion_s": 64.5,
  "paquetes_ble": 128, "paquetes_ble_perdidos": 1, "recepcion_semaforo_pct": 96,
  "rssi_semaforo_medio_dbm": -64, "rssi_semaforo_min_dbm": -78, "semaforo_ble_id": 1,
  "brujula_sin_calibrar": false
}
```

| Campo | Qué es y cómo se mide |
|---|---|
| `lat`, `lon`, `precision_m` | Ubicación del **semáforo**: la lectura del GPS del teléfono más precisa mientras la persona espera en la esquina, antes del verde. `null` si no hubo GPS. |
| `semaforo_id` | Semáforo guardado en el teléfono a menos de 25 m. Si no había, la app lo crea y manda también un evento `semaforo`. `null` sin GPS confiable o en modo simulado. |
| `espera_s` | Desde la detección hasta el primer paquete "permitido" (verde y bien orientado). `null` si nunca hubo verde. |
| `cruzo`, `duracion_cruce_s` | El cruce empieza con el primer "permitido". Termina cuando el GPS ubica a la persona a 12 m de la esquina **en la dirección del rumbo del brazalete**, con 60° de margen. Alejarse por la misma acera no cuenta. |
| `verde_restante_s` | Segundos de verde que quedaban al empezar el cruce. |
| `rumbo_cruce` | Rumbo del brazalete al empezar el cruce, en grados magnéticos. |
| `veces_mal_orientado` | Cuántas veces el brazalete pasó a "mal orientado". |
| `simulado` | `true` si salió del brazalete simulado de la app: no mostrarlo en la web. |
| `paquetes_ble` | Paquetes de telemetría BLE que recibió el teléfono en la sesión. |
| `paquetes_ble_perdidos` | Notificaciones BLE que no llegaron, por los saltos de la secuencia `n` del paquete (ver `docs/ble-protocol.md`). |
| `recepcion_semaforo_pct` | Porcentaje de paquetes ESP-NOW del semáforo que recibió el brazalete (promedio de `rx` / 20). 100 = ninguno perdido. |
| `rssi_semaforo_medio_dbm`, `rssi_semaforo_min_dbm` | Señal del semáforo en el brazalete (`rs`): promedio y peor valor. Sirve para medir el alcance. |
| `semaforo_ble_id` | `id` del semáforo que siguió el brazalete. |
| `brujula_sin_calibrar` | `true` si el brazalete avisó que la brújula no estaba calibrada. |

Las métricas de enlaces son `null` (y `brujula_sin_calibrar` es `false`) con un firmware que
envía la telemetría BLE de la versión 1.

Límites conocidos:
- El GPS sólo se sigue con la app abierta.
- En avenidas de más de 12 m, el cruce se da por terminado antes de llegar al otro lado.

La lista completa de campos está en `ResumenCruce`, en `app-movil/src/telemetria/sesion.ts`.

#### `semaforo`
Se envía cada vez que se crea, edita o borra un semáforo en el teléfono, a mano o al
detectarlo el brazalete.

```json
{ "v": 3, "id": "mh3k2c1r5e7f", "tipo": "semaforo", "dispositivo": "mh3k2a9x1c4f",
  "momento": 1793300000000, "accion": "creado", "semaforo_id": 3,
  "nombre": "Semáforo en Calle 26 13", "lat": 4.60971, "lon": -74.08175,
  "precision_m": 6, "origen": "brazalete" }
```

- `accion`: `creado`, `editado` o `borrado`. `borrado` sólo trae `semaforo_id`.
- `origen` (`manual` o `brazalete`) sólo va en `creado`.
- `semaforo_id` es local de cada teléfono: un semáforo se identifica por
  `dispositivo` + `semaforo_id`, y su estado actual es el último evento de esa pareja.

#### `prueba`
Lo manda el botón de Diagnóstico. Trae `mensaje`. El servidor sólo tiene que aceptarlo, con un `2xx`.

---

## Dependencias
- Un servicio web en Render con HTTPS (Render lo da por defecto; iOS no deja usar `http://`).
- Una base de datos para guardar los eventos. Revisar en render.com los límites del plan
  gratuito de la base de datos antes de depender de ella.

### Recomendaciones para el servidor
- Una tabla `eventos (id TEXT PRIMARY KEY, tipo TEXT, dispositivo TEXT, momento BIGINT, recibido TIMESTAMP, datos JSONB)`
  basta para empezar. Las vistas para la web (semáforos actuales, promedios por semáforo)
  se sacan de ahí.
- Guardar el token en una variable de entorno de Render y compararlo con el encabezado
  `Authorization`. Para la web, usar rutas de **sólo lectura** sin ese token, por ejemplo
  `GET /api/semaforos`, que devuelvan sólo lo que se puede mostrar en público (nada de `dispositivo`).
- Validar los tipos y responder `400` a lo mal formado; con cualquier otro error, `5xx`, para
  que la app lo reintente.
- La app no necesita CORS (las peticiones nativas no lo usan). La web sí, si está en otro dominio.

---

## Ejemplo de Uso
Probar el servidor desde la terminal, igual que lo haría la app:

```bash
curl -i -X POST "https://acpi.onrender.com/api/eventos" \
  -H "Content-Type: application/json" -H "Authorization: Bearer $ACPI_TOKEN" \
  -d '{"eventos":[{"v":3,"id":"prueba-curl-1","tipo":"prueba","dispositivo":"curl","momento":1793300000000,"mensaje":"hola"}]}'
```

Si se manda dos veces, el servidor debe responder `2xx` las dos veces y guardar un solo evento.
