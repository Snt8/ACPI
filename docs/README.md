# Especificaciones de Protocolos, Telemetría e Integración (ACPI)

## Descripcion General
Este directorio contiene la documentación técnica y las especificaciones de arquitectura para la comunicación de red, los protocolos inalámbricos y la integración con el sistema de telemetría y visualización de datos en la nube. Define cómo interactúan la Pulsera Inteligente, el Semáforo, la Aplicación Móvil y el panel de control de Grafana Cloud.

## Contenido
El directorio incluye los siguientes documentos técnicos especializados:
- **[ble-protocol.md](file:///c:/Users/sntav/programacion/ACPI/docs/ble-protocol.md)**: Especificación técnica detallada del protocolo de comunicación Bluetooth Low Energy (BLE) entre la Pulsera (servidor GATT) y el dispositivo Android (cliente GATT).
- **[grafana-setup.md](file:///c:/Users/sntav/programacion/ACPI/docs/grafana-setup.md)**: Guía paso a paso para la configuración del ingestor de datos en Grafana Cloud, mapeo de variables JSON de telemetría y diseño de paneles recomendados.

## Explicacion
### 1. Topología Inalámbrica Dual
El sistema ACPI utiliza una separación estricta de protocolos según el dispositivo:
- **Pulsera (ESP32-C3)**: Utiliza **BLE únicamente** para comunicarse con el celular del peatón. Tiene prohibido levantar la pila Wi-Fi para optimizar la batería.
- **Semáforo (ESP32 DevKit V1)**: Transmite periódicamente su orientación y estado magnético a la pulsera a través de **ESP-NOW** en modo broadcast (2Hz).
- **Celular (App Android)**: Recibe los datos vía BLE y, cuando finaliza una sesión de cruce, utiliza la red móvil (Wi-Fi o Datos) para enviar las métricas agregadas por HTTP Push (POST) a Grafana Cloud.

### 2. Formato del Payload de Telemetría
Para reducir la carga en la red móvil del usuario, el JSON de sesión se mantiene en una sola jerarquía (plano), facilitando la ingesta directa en bases de datos de series temporales (como InfluxDB o Prometheus):
- **`device_id`**: Identificador de hardware.
- **`session_id`**: UUID generado por la aplicación para rastrear el cruce actual.
- **`status`**: Estado final codificado numéricamente (`0` = Espera, `1` = Cruce seguro/alineado, `2` = Desalineado, `3` = Alerta de pánico).
- **Métricas físicas**: Pasos (`steps`), duración (`duration_sec`), y geolocalización (`lat` y `lon`).

## Dependencias
Para implementar e interactuar con estos protocolos e integraciones, se requiere:
- **Pila BLE v4.2 o superior**: Compatible con GATT en ESP32-C3 y el sistema operativo Android.
- **Servicio Grafana Cloud (Free Tier)**: O bien un gateway/broker local (Mosquitto/MQTT) capaz de procesar peticiones HTTP JSON.

## Ejemplo de Uso
Ejemplo de cómo reportar manualmente un evento de cruce de prueba al endpoint HTTP de Grafana Cloud (o gateway compatible) utilizando `curl`:

```bash
curl -X POST "https://influx-prod-13-prod-us-east-0.grafana.net/api/v1/push" \
  -H "Authorization: Bearer glc_eyJ..." \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "ACPI_Pulsera_Test",
    "session_id": "99999999-9999-9999-9999-999999999999",
    "status": 1,
    "timestamp": 1780704000000,
    "duration_sec": 28,
    "waiting_sec": 8,
    "steps": 32,
    "panics": 0,
    "lat": 19.4326,
    "lon": -99.1332
  }'
```
