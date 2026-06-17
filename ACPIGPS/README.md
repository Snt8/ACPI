# ACPI Android Mobile Application (ACPIGPS)

## Descripcion General
La aplicación móvil **ACPIGPS** actúa como el puente de comunicaciones central del ecosistema ACPI. Su función principal es conectarse mediante **Bluetooth Low Energy (BLE)** a la Pulsera Inteligente (ESP32-C3), procesar sus notificaciones de estado y orientación magnética en tiempo real, guiar al peatón de manera visual/auditiva y reportar las métricas agregadas de cada sesión de cruce de forma segura y resiliente hacia el backend de **Grafana Cloud** mediante payloads JSON simplificados.

## Contenido
La aplicación está organizada en torno a los siguientes archivos y submódulos principales en `app/src/main/java/com/example/acpigps/`:
- **[MainActivity.kt](file:///c:/Users/sntav/programacion/ACPI/ACPIGPS/app/src/main/java/com/example/acpigps/MainActivity.kt)**: Punto de entrada de la UI y gestión de navegación basada en Jetpack Compose.
- **[BLEManager.kt](file:///c:/Users/sntav/programacion/ACPI/ACPIGPS/app/src/main/java/com/example/acpigps/BLEManager.kt)**: Manejador GATT cliente que gestiona el escaneo, emparejamiento, suscripción a notificaciones y envío de comandos a la pulsera.
- **[GrafanaReporter.kt](file:///c:/Users/sntav/programacion/ACPI/ACPIGPS/app/src/main/java/com/example/acpigps/GrafanaReporter.kt)**: Módulo encargado del formateo de telemetría y su envío mediante HTTP POST asíncrono con un buffer resiliente ante pérdidas de conectividad.
- **[MonitoringService.kt](file:///c:/Users/sntav/programacion/ACPI/ACPIGPS/app/src/main/java/com/example/acpigps/MonitoringService.kt)**: Servicio en primer plano (Foreground Service) que garantiza que las conexiones BLE y de red sigan activas incluso si la pantalla se apaga.
- **[WalkDirectionDetector.kt](file:///c:/Users/sntav/programacion/ACPI/ACPIGPS/app/src/main/java/com/example/acpigps/WalkDirectionDetector.kt)**: Detector lógico de pasos y patrones de desplazamiento.
- **[SessionLogger.kt](file:///c:/Users/sntav/programacion/ACPI/ACPIGPS/app/src/main/java/com/example/acpigps/SessionLogger.kt)**: Registro local en formato de texto dentro del almacenamiento del dispositivo.
- **[PulseraData.kt](file:///c:/Users/sntav/programacion/ACPI/ACPIGPS/app/src/main/java/com/example/acpigps/PulseraData.kt)**: Modelo de datos que mapea la telemetría recibida por BLE.

## Explicacion
### 1. Comunicación BLE (GATT Cliente)
La app implementa el perfil GATT cliente interactuando con los UUIDs definidos en el firmware de la pulsera:
- **Servicio Principal**: `12345678-1234-1234-1234-123456789abc`
- **Característica TX (Notificación)**: `87654321-4321-4321-4321-cba987654321` (Recibe el JSON con rumbo, alineación y estado de cruce cada 500 ms).
- **Característica RX (Escritura)**: `11111111-2222-3333-4444-555555555555` (Envía comandos de control o cuadrantes de rumbo al ESP32).

### 2. Resiliencia de Telemetría (GrafanaReporter)
Para cumplir con los estándares de bajo consumo y resiliencia, `GrafanaReporter` implementa un buffer circular en memoria (`ConcurrentLinkedQueue`) de hasta 100 elementos. 
- Al completarse una sesión, el JSON se añade a la cola y se intenta realizar el POST asíncrono usando `OkHttpClient`.
- Si la conexión de red falla, el payload se mantiene en cola y se reintenta en el próximo envío.
- Si las credenciales (`grafana_url` y `grafana_token`) no se encuentran configuradas en `SharedPreferences` (bajo el archivo `app_prefs`), el reporte falla silenciosamente para no bloquear la ejecución principal.

## Dependencias
Las principales dependencias declaradas en el archivo de construcción Gradle de la app son:
- **OkHttp (v4.x+)**: Para las peticiones de red asíncronas y seguras hacia Grafana Cloud.
- **Android Bluetooth Suite**: APIs nativas de Android SDK para operaciones de escaneo y conectividad GATT.
- **Jetpack Compose**: Framework UI moderno para la renderización de pantallas.
- **Lifecycle & LiveData**: Gestión del estado reactivo entre la lógica de negocio y la interfaz de usuario.

## Ejemplo de Uso
A continuación se ilustra cómo el servicio interactúa con el reportero de telemetría al finalizar un cruce peatonal:

```kotlin
// Instanciación del reportero
val reporter = GrafanaReporter(context)

// Creación de las métricas de la sesión
val metrics = JSONObject().apply {
    put("duration_sec", 30)
    put("waiting_sec", 12)
    put("steps", 45)
    put("panics", 0)
    put("lat", 19.4326)
    put("lon", -99.1332)
}

// Envío asíncrono y resiliente
reporter.sendSessionTelemetry(
    deviceId = "ACPI_Pulsera",
    sessionId = UUID.randomUUID().toString(),
    status = 1, // 1 = Cruce exitoso y alineado
    metrics = metrics
)
```
