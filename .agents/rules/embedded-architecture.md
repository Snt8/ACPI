---
trigger: always_on
---

# Global Rule: Embedded Architecture & Low-Power Hardware Standards

## Purpose
This rule enforces strict constraints on how the AI agent writes, refactors, or modularizes firmware for the ACPI wearable ecosystem (Pulsera and Semáforo). It ensures edge-computing optimization on the local nodes, enforces correct wireless protocols per device, and mandates ultra-low power consumption.

## 1. Node Definition & Wireless Topologies
The agent must strictly separate code implementations based on the clear, dual-hardware roles defined below:

### A. The Wristband (Pulsera - ESP32-C3 Super Mini)
- **Role:** High-efficiency edge sensing, pedestrian orientation tracking, and local haptic/visual feedback.
- **Wireless Protocol:** **Bluetooth Low Energy (BLE) ONLY** to communicate with the Android App. Wi-Fi stack generation on the wristband is strictly prohibited to maximize battery life.
- **Edge Analytics:** Sensor data from the MPU6050 (Accelerometer/Gyroscope) and QMC5883L (Compass) must be filtered and fused *locally* on the chip before transmitting. Do not stream raw, noisy high-frequency sensor bytes over BLE.

### B. The Traffic Light (Semáforo - ESP32 DevKit V1)
- **Role:** Structural control and environment sync.
- **Wireless Protocols:** - **ESP-NOW** for ultra-low latency, direct peer-to-peer communication with the wristband (if standalone bypass is required).
  - **Wi-Fi (MQTT)** strictly for pushing aggregated session metadata to the telemetry gateway (Grafana backend).

## 2. Memory & Resource Constraints (ESP32-C3 & FreeRTOS)
- **Static Allocations:** Since the ESP32-C3 Super Mini has limited SRAM, prefer Stack allocation over dynamic memory (`malloc`/`new`) to guarantee system uptime during extended runtime.
- **String Handling:** Wrap immutable telemetry string templates inside Flash memory macros (`F()` or `const char* PROGMEM`) to free up critical RAM.
- **Non-Blocking Logic:** All hardware polling loops must utilize non-blocking `millis()` state checks or native FreeRTOS Tasks (`vTaskDelay`). Blocking delays (`delay()`) inside core execution loops are strictly prohibited.

## 3. Power Management & Wearable States
The wristband must be designed as a wearable. The agent must implement a strict Finite State Machine (FSM):
- **States:** `BOOTING`, `STANDBY_LOW_POWER` (Light/Deep Sleep when no crossing session is active), `SENSING_CROSSING`, and `ALERT_PANIC`.
- **Hardware Interrupts:** Wake-up routines and panic triggers must be tied to Hardware ISRs (Interrupt Service Routines), avoiding continuous button polling.

## 4. Code Modularity Requirements
The agent must structure firmware across folders cleanly:
- `src/sensors/` -> Hardware abstraction layers for MPU6050 and QMC5883L.
- `src/wireless/` -> Dedicated BLE services (for Wristband) or ESP-NOW/MQTT routines (for Semáforo).
- `src/control/` -> Main FSM, state transitions, and safety algorithms.