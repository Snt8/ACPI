# Especificaciones de Protocolos, Telemetría e Integración (ACPI)

## Descripcion General
Este directorio contiene la documentación técnica y las especificaciones de arquitectura para la comunicación de red, los protocolos inalámbricos y la integración con el sistema de telemetría y visualización de datos en la nube. Define cómo interactúan el Brazalete Inteligente, el Semáforo, la Aplicación Móvil y el panel de control de Grafana Cloud.

## Contenido
El directorio incluye los siguientes documentos técnicos especializados:
- **[ble-protocol.md](file:///c:/Users/sntav/programacion/ACPI/docs/ble-protocol.md)**: Especificación técnica detallada del protocolo de comunicación Bluetooth Low Energy (BLE) entre el Brazalete (servidor GATT) y el dispositivo Android (cliente GATT).
- **[grafana-setup.md](file:///c:/Users/sntav/programacion/ACPI/docs/grafana-setup.md)**: Guía paso a paso para la configuración del ingestor de datos en Grafana Cloud, mapeo de variables JSON de telemetría y diseño de paneles recomendados.

## Explicacion
### 1. Topología Inalámbrica Dual
El sistema ACPI utiliza una separación estricta de protocolos según el dispositivo:
- **Brazalete (ESP32-C3)**: Utiliza **BLE únicamente** para comunicarse con el celular del peatón. Tiene prohibido levantar la pila Wi-Fi para optimizar la batería.
- **Semáforo (ESP32 DevKit V1)**: Transmite periódicamente su orientación y estado magnético al brazalete a través de **ESP-NOW** en modo broadcast (2Hz).
- **Celular (app móvil)**: Recibe los datos vía BLE y, cuando finaliza una sesión de cruce, utiliza la red móvil (Wi-Fi o Datos) para enviar las métricas agregadas por HTTP Push (POST) a Grafana Cloud.

### 2. Formato de la telemetría
La app manda a Loki (Grafana Cloud) dos tipos de evento en JSON plano:
- `cruce`, al terminar cada sesión, con la ubicación del semáforo, la espera y la duración del cruce.
- `semaforo`, cuando se crea, edita o borra un semáforo guardado.

El contrato completo, y cómo se mide cada campo, está en [`grafana-setup.md`](grafana-setup.md).

## Dependencias
Para implementar e interactuar con estos protocolos e integraciones, se requiere:
- **Pila BLE v4.2 o superior**: Compatible con GATT en ESP32-C3 y el sistema operativo Android.
- **Servicio Grafana Cloud (Free Tier)**: Loki recibe los eventos directo del teléfono; no hace falta un servidor propio.

## Ejemplo de Uso
Enviar un evento de prueba a Loki con `curl`, con el usuario y el token de `app-movil/.env`. Antes, cambiar `1793300000000000000` por la hora actual en nanosegundos; Loki rechaza eventos muy viejos.

```bash
curl -u "$LOKI_USUARIO:$LOKI_TOKEN" -H "Content-Type: application/json" \
  "https://logs-prod-XXX.grafana.net/loki/api/v1/push" \
  -d '{"streams":[{"stream":{"app":"acpi","tipo":"prueba"},"values":[["1793300000000000000","{\"v\":2,\"tipo\":\"prueba\"}"]]}]}'
```

Para usarlo desde la app: Ajustes → Diagnóstico → **Enviar dato de prueba**.
