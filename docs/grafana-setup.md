# Configuración de Telemetría con Grafana Cloud

Este documento describe cómo configurar una cuenta gratuita en **Grafana Cloud** y conectarla con la aplicación Android de **ACPI v2.0** para visualizar la telemetría del sistema en tiempo real.

---

## 1. Crear una Cuenta y Obtener Credenciales

1. Regístrate en una cuenta gratuita de **Grafana Cloud** en [grafana.com](https://grafana.com/).
2. En tu consola de Grafana, navega a **Access Policies** y genera un nuevo Token de API con permisos de escritura.
3. Toma nota del endpoint HTTP proporcionado. Si utilizas el plugin de InfluxDB o un recolector JSON compatible, el endpoint típicamente tiene un formato como:
   `https://<grafana-instance-host>/api/v1/push` o `https://<influx-host>/api/v2/write`

---

## 2. Configuración en la App Android

El `GrafanaReporter` de la aplicación Android carga de manera dinámica la URL del servidor y el Token desde las SharedPreferences. Puedes configurarlos programáticamente o a través de la UI de la aplicación (preferencias) guardando las siguientes claves en el archivo `app_prefs`:

- **Key:** `grafana_url`  
  **Value:** La URL de push de Grafana Cloud (ejemplo: `https://influx-prod-13-prod-us-east-0.grafana.net/api/v1/push`)
- **Key:** `grafana_token`  
  **Value:** Tu Token de API de Grafana (ejemplo: `glc_eyJ...`)

*Si estas claves no están configuradas en SharedPreferences, los datos de telemetría fallarán silenciosamente para no interrumpir el flujo de seguridad principal del usuario.*

---

## 3. Contrato de Datos (Payload JSON)

Cada sesión de cruce finalizada envía un payload JSON aplanado con el siguiente formato:

```json
{
  "device_id": "ACPI_Pulsera",
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
- **`device_id`**: Identificador único de la pulsera.
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
