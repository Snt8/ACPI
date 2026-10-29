# Especificaciones de Protocolos, Telemetría e Integración (ACPI)

## Descripcion General
Este directorio contiene la documentación técnica y las especificaciones de arquitectura para la comunicación de red, los protocolos inalámbricos y la integración con el sistema de telemetría y visualización de datos en la nube. Define cómo interactúan el Brazalete Inteligente, el Semáforo, la Aplicación Móvil y el servidor web de ACPI en Render.

## Contenido
El directorio incluye los siguientes documentos técnicos especializados:
- **[ble-protocol.md](file:///c:/Users/sntav/programacion/ACPI/docs/ble-protocol.md)**: Especificación técnica detallada del protocolo de comunicación Bluetooth Low Energy (BLE) entre el Brazalete (servidor GATT) y el dispositivo Android (cliente GATT).
- **[servidor-api.md](servidor-api.md)**: Contrato entre la app móvil y el servidor web de ACPI en Render: rutas, formato de los eventos y cómo se mide cada campo.

## Explicacion
### 1. Topología Inalámbrica Dual
El sistema ACPI utiliza una separación estricta de protocolos según el dispositivo:
- **Brazalete (ESP32-C3)**: Utiliza **BLE únicamente** para comunicarse con el celular del peatón. Tiene prohibido levantar la pila Wi-Fi para optimizar la batería.
- **Semáforo (ESP32 DevKit V1)**: Transmite periódicamente su orientación y estado magnético al brazalete a través de **ESP-NOW** en modo broadcast (2Hz).
- **Celular (app móvil)**: Recibe los datos vía BLE y, cuando finaliza una sesión de cruce, utiliza la red móvil (Wi-Fi o Datos) para enviar las métricas agregadas por HTTPS (POST) al servidor web de ACPI en Render.

### 2. Formato de la telemetría
La app manda al servidor de ACPI dos tipos de evento en JSON plano:
- `cruce`, al terminar cada sesión, con la ubicación del semáforo, la espera y la duración del cruce.
- `semaforo`, cuando se crea, edita o borra un semáforo guardado.

El contrato completo, y cómo se mide cada campo, está en [`servidor-api.md`](servidor-api.md).

## Dependencias
Para implementar e interactuar con estos protocolos e integraciones, se requiere:
- **Pila BLE v4.2 o superior**: Compatible con GATT en ESP32-C3 y el sistema operativo Android.
- **Servidor web en Render**: recibe los eventos de la app por HTTPS (ver `servidor-api.md`).

## Ejemplo de Uso
Enviar un evento de prueba al servidor con `curl`, con la URL y el token de `app-movil/.env`:

```bash
curl -i -X POST "https://acpi.onrender.com/api/eventos" \
  -H "Content-Type: application/json" -H "Authorization: Bearer $ACPI_TOKEN" \
  -d '{"eventos":[{"v":3,"id":"prueba-curl-1","tipo":"prueba","dispositivo":"curl","momento":1793300000000}]}'
```

Para usarlo desde la app: Ajustes → Diagnóstico → **Enviar dato de prueba**.
