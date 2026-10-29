# Configuración de Telemetría con Grafana Cloud

Este documento describe cómo configurar una cuenta gratuita en **Grafana Cloud**, conectarla con la app móvil de **ACPI** (`app-movil/`) y mostrar los semáforos y cruces en un mapa que se puede publicar en la web de ACPI.

No hace falta un servidor propio: el teléfono escribe directo en **Loki** (la base de logs de Grafana Cloud) y la web muestra un tablero de Grafana publicado.

```text
[Brazalete] ──BLE──► [App móvil] ──HTTPS (logs:write)──► [Loki en Grafana Cloud] ──► [Tablero publicado] ──► [Web de ACPI]
                     GPS + SQLite (cola sin red)
```

---

## 1. Crear la cuenta y las credenciales

1. Crear una cuenta gratuita en [grafana.com](https://grafana.com/). Se crea un *stack* con Loki incluido.
2. En el portal de Grafana Cloud → tu stack → **Loki** → **Details**, anotar:
   - la **URL** (por ejemplo `https://logs-prod-024.grafana.net`);
   - el **User** (un número).
3. En **Access Policies**, crear una política con **sólo el permiso `logs:write`** y generarle un token (`glc_…`).
4. En `app-movil/`, copiar `.env.example` a `.env` y completar `EXPO_PUBLIC_LOKI_URL`, `EXPO_PUBLIC_LOKI_USUARIO` y `EXPO_PUBLIC_LOKI_TOKEN`. `.env` no se sube a git. Tiene que estar en la computadora que corre `npm start` o compila la app, porque Expo copia esas variables al código de la app.
5. En la app: **Ajustes → Diagnóstico → Enviar dato de prueba**. Si aparece "1 eventos enviados", ya funciona.

**Por qué un token de sólo escritura:** el token queda dentro de la app y alguien podría sacarlo. Con `logs:write` sólo podría escribir registros, no leer ni borrar los datos.

---

## 2. Cómo envía la app

- Cada evento se guarda primero en una cola SQLite del teléfono (`envios_pendientes`). Se envía al abrir la app, al volver a ella, cada minuto y apenas se encola algo. Sin red, espera, con un máximo de 1000 eventos.
- `POST {URL}/loki/api/v1/push` con autenticación HTTP Basic `usuario:token`.
- Cada tipo de evento es un *stream* con las etiquetas `app="acpi"` y `tipo` (`cruce`, `semaforo` o `prueba`). La línea de log es el JSON de la sección 3.
- Si el usuario desactiva **Ajustes → Compartir cruces con ACPI**, no se encola nada y se borra lo pendiente.

Código: `app-movil/src/telemetria/`.

---

## 3. Contrato de datos (versión 2)

Todos los eventos llevan:
- `v`: versión del formato (2).
- `tipo`.
- `dispositivo`: seudónimo aleatorio del teléfono, que no identifica a la persona.
- `momento`: ms desde epoch.

### Evento `cruce`

Se envía al terminar cada sesión. Una sesión va desde que el brazalete encuentra un semáforo y se conecta al teléfono hasta que se desconecta, 30 s después de perderlo.

```json
{
  "v": 2, "tipo": "cruce", "dispositivo": "mh3k2a9x1c4f", "momento": 1793300000000,
  "sesion": "mh3k2b0q8d2e", "inicio": 1793299940000, "simulado": false,
  "lat": 4.60971, "lon": -74.08175, "precision_m": 6, "semaforo_id": 3,
  "espera_s": 21.4, "cruzo": true, "duracion_cruce_s": 13.2,
  "verde_restante_s": 25, "rumbo_cruce": 92, "veces_mal_orientado": 1,
  "duracion_sesion_s": 64.5
}
```

| Campo | Qué es y cómo se mide |
|---|---|
| `lat`, `lon`, `precision_m` | Ubicación del **semáforo**. Es la lectura del GPS del teléfono más precisa mientras la persona espera en la esquina, antes del verde. Si no hubo GPS, `null`. |
| `semaforo_id` | Semáforo de la lista del teléfono a menos de 25 m. Si no había ninguno, se guarda uno nuevo ("Detectado por el brazalete"). `null` sin GPS confiable o en modo simulado. |
| `espera_s` | Desde la detección hasta el primer paquete "permitido" (verde y bien orientado). `null` si nunca hubo verde. |
| `cruzo`, `duracion_cruce_s` | El cruce empieza con el primer "permitido". Termina cuando el GPS ubica a la persona a 12 m de la esquina **en la dirección del rumbo del brazalete**, con hasta 60° de margen. Alejarse por la misma acera no cuenta. Se ignoran lecturas con más de 25 m de error. |
| `verde_restante_s` | Segundos de verde que quedaban al empezar el cruce (`tr` del brazalete). |
| `rumbo_cruce` | Rumbo del brazalete al empezar el cruce, en grados magnéticos. |
| `veces_mal_orientado` | Cuántas veces el brazalete pasó a "mal orientado". |
| `simulado` | `true` si salió del brazalete simulado: filtrarlo en los tableros. |

Límites conocidos:
- La sesión sólo sigue el GPS con la app abierta; el funcionamiento en segundo plano está pendiente.
- En avenidas de más de 12 m, el cruce se da por terminado antes de llegar al otro lado.

### Evento `semaforo`

Se envía cada vez que se crea, edita o borra un semáforo en el teléfono, a mano o al detectarlo el brazalete.

```json
{ "v": 2, "tipo": "semaforo", "dispositivo": "mh3k2a9x1c4f", "momento": 1793300000000,
  "accion": "creado", "semaforo_id": 3, "nombre": "Semáforo en Calle 26 13", "lat": 4.60971,
  "lon": -74.08175, "precision_m": 6, "origen": "brazalete" }
```

- `accion` es `creado`, `editado` o `borrado`. `borrado` sólo trae `semaforo_id`.
- `origen` (`manual` o `brazalete`) sólo va en `creado`.
- Un semáforo se identifica por `dispositivo` + `semaforo_id`; su estado actual es el último evento de esa pareja.

---

## 4. Mapa en Grafana

1. **Explore** → fuente Loki → consulta para los cruces:
   ```logql
   {app="acpi", tipo="cruce"} | json | simulado = "false" | lat != ""
   ```
   Y para los semáforos:
   ```logql
   {app="acpi", tipo="semaforo"} | json | accion = "creado"
   ```
2. Crear un tablero con un panel **Geomap**:
   - Capa de marcadores con *Location mode* = *Coords*, latitud `lat` y longitud `lon`.
   - Transformación **Extract fields** (formato JSON) sobre la línea si los campos no aparecen solos.
3. Otros paneles útiles:
   - **Stat** con el promedio de `duracion_cruce_s` y `espera_s`.
   - **Table** con `nombre`, `espera_s` y `duracion_cruce_s` por semáforo.

## 5. Mostrarlo en la web de ACPI

En el tablero: **Share → Share externally** (antes "Public dashboard"). Grafana da un enlace público de sólo lectura que corre únicamente las consultas guardadas del tablero. Así la web no necesita ningún token.

En la web se pone ese enlace o un `<iframe src="…">` con él. Hay que comprobar en el stack que el panel Geomap con Loki se vea en el tablero público.

Alternativa para más adelante: una web que lea Loki directamente necesitaría un token de lectura, y ese token no puede ir en una página pública. Para eso sí haría falta un servidor o una función *serverless* intermedia.
