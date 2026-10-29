# Configuración de Telemetría con Grafana Cloud

Este documento describe cómo configurar una cuenta gratuita en **Grafana Cloud** y conectarla con la app móvil de **ACPI** para visualizar la telemetría del sistema en tiempo real.

---

## 1. Crear una Cuenta y Obtener Credenciales

1. Regístrate en una cuenta gratuita de **Grafana Cloud** en [grafana.com](https://grafana.com/).
2. En tu consola de Grafana, navega a **Access Policies** y genera un nuevo Token de API con permisos de escritura.
3. Toma nota del endpoint HTTP proporcionado. Si utilizas el plugin de InfluxDB o un recolector JSON compatible, el endpoint típicamente tiene un formato como:
   `https://<grafana-instance-host>/api/v1/push` o `https://<influx-host>/api/v2/write`

---

## 2. Envío desde la app móvil

No hace falta un servidor propio: el teléfono envía cada sesión de cruce directo al endpoint
de **Loki** de Grafana Cloud.

- **URL:** `https://logs-prod-<región>.grafana.net/loki/api/v1/push` (aparece en la tarjeta de
  Loki de tu stack).
- **Autenticación:** HTTP Basic con el *User* de Loki (un número) y el token.
- **Cuerpo:** un *stream* con las etiquetas `app="acpi"` y `device_id`, y como línea de log el
  JSON de la sección 3:
  `{"streams":[{"stream":{"app":"acpi","device_id":"ACPI_Brazalete"},"values":[["<ns>","<json>"]]}]}`

El token queda dentro de la app, así que se crea con una *access policy* que **sólo tenga el
permiso `logs:write`**: quien lo extraiga puede escribir registros, pero no leerlos ni borrarlos.
Si faltan las credenciales, la telemetría se descarta sin interrumpir los avisos al usuario.

> Estado (29 oct 2026): la app Kotlin que hacía este envío se retiró; en `app-movil/` el envío
> está pendiente (ver `docs/guia-app-invidentes.md`).

---

## 3. Contrato de Datos (Payload JSON)

Cada sesión de cruce finalizada envía un payload JSON aplanado con el siguiente formato:

```json
{
  "device_id": "ACPI_Brazalete",
  "session_id": "8fa538e1-5e93-4a11-8254-cd6ef2802d28",
  "status": 1,
  "timestamp": 1780704000000,
  "duration_sec": 35,
  "waiting_sec": 15,
  "steps": 24,
  "panics": 0,
  "allowed_cruce": 1,
  "lat": 19.4326,
  "lon": -99.1332
}
```

### Campos Enviados:
- **`device_id`**: Identificador único del brazalete.
- **`session_id`**: Identificador único (UUID) de la sesión de cruce del peatón.
- **`status`**: Estado final de la sesión:
  - `0` = Espera / Seguro
  - `1` = Cruce exitoso y permitido
  - `2` = Orientación incorrecta
  - `3` = Pánico/Alerta crítica
- **`duration_sec`**: Duración total del cruce en segundos.
- **`waiting_sec`**: Tiempo total que esperó el peatón antes de poder cruzar (st = 0).
- **`steps`**: Número de pasos detectados durante el cruce.
- **`panics`**: Número de veces que el usuario presionó el botón de pánico.
- **`lat` / `lon`**: Coordenadas GPS de finalización del cruce.

---

## 4. Paneles Recomendados para el Dashboard

Una vez que los datos fluyen a tu base de datos de series temporales en Grafana, se recomienda configurar los siguientes paneles:

### A. Mapa de Calor / Geolocalización (Geomap)
- **Visualización:** Geomap
- **Campos:** `lat` y `lon`
- **Utilidad:** Muestra en un mapa interactivo todos los cruces del usuario, marcando con colores (verde, amarillo, rojo) los cruces exitosos, fallidos o de pánico.

### B. Línea de Tiempo de Estados (State Timeline)
- **Visualización:** State Timeline
- **Campos:** `status`
- **Mapeo de Colores:**
  - `0` (Espera) -> Gris
  - `1` (Cruce Exitoso) -> Verde
  - `2` (Desalineado) -> Naranja
  - `3` (Pánico) -> Rojo Brillante

### C. Estadísticas de Movilidad (Stat / Gauge)
- **Visualización:** Stat o Gauge
- **Campos:** `steps` (Pasos totales caminados), `waiting_sec` (Tiempo de espera promedio en semáforos)
- **Utilidad:** Indicadores de rendimiento del usuario y eficiencia de los semáforos.

### D. Historial de Pánico (Table)
- **Visualización:** Table
- **Filtro:** `panics > 0` o `status == 3`
- **Utilidad:** Un registro claro para que familiares o cuidadores identifiquen las intersecciones más riesgosas donde el usuario requirió activar la alerta de pánico.
