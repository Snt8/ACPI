---
trigger: always_on
---

# Global Rule: Telemetry Protocol & Grafana Cloud Standards

## Purpose
This rule enforces the architectural decision to drop custom, from-scratch Web Dashboards and custom Firebase integrations. The AI agent must redirect all data visualization efforts into designing standardized, ultra-lightweight telemetry streams that can be easily fed into Grafana Cloud or an open-source IoT Gateway.

## 1. Complete Custom UI Ban
- **No Web Interfaces:** The agent is strictly forbidden from writing HTML, CSS, JavaScript panels, or setting up complex web hosting configurations for data tracking. 
- **No Direct Firebase UI Binding:** Any legacy Firebase setup must be evaluated only as an ingestion backend, never as a custom frontend renderer.

## 2. Telemetry Architecture & Data Contracts
The telemetry stream originates from the Pulsera (via BLE to the Android App) or the Traffic Light (via Wi-Fi/MQTT). The agent must guarantee that the transmission formatting is ready for Time-Series Databases (such as InfluxDB, Prometheus, or Graphite) used by Grafana.
- **Flattened JSON Payloads:** Keep network overhead minimal. Propose single-tier JSON keys.
- **Mandatory Fields per Session:** Every telemetry event must explicitly contain:
  - `device_id` (String - Unique Identifier for the device node).
  - `session_id` (String/Long - Identifies the pedestrian's crossing sequence).
  - `status` (Integer/Enum - State mapping: 0=Safe, 1=Approaching, 2=Danger, 3=Panic).
  - `metrics` (Numeric values from the MPU6050 Accelerometer/Gyroscope and QMC5883L Compass).

## 3. Data Ingestion Architecture Guidance
When modifying the Android App or the Traffic Light backend to emit metrics to Grafana, the agent must advise the user on utilizing standard lightweight ingestors:
- **MQTT Bridge:** Push metrics to an MQTT broker (e.g., HiveMQ Free, Mosquitto) where Grafana's IoT plugins can fetch them.
- **Direct HTTP Push:** Use Grafana Cloud's native Prometheus Remote Write API or InfluxDB HTTP API via simple web requests directly from the Android App or a middleware collector.

## 4. Telemetry Resilience
The system must never stall. The firmware or the Android app code written by the agent must handle connection timeouts asynchronously. If the Grafana gateway/MQTT broker is unreachable, data should either fail silently or be briefly stored in a local ring-buffer (RAM/Flash) to avoid crashing the core safety loop of the ACPI device.