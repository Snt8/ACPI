---
trigger: always_on
---

# Global Rule: PlatformIO, C++ & Repository Automation Standards

## Purpose
This rule strictly governs how the AI agent builds, modifies, and validates firmware across the ACPI architecture. It enforces strict compliance with PlatformIO Core CLI tools, prevents unoptimized memory allocations on restricted chips like the ESP32-C3 Super Mini, and structures repository tasks natively via `just`.

## 1. Environment & Project Management via PlatformIO
The agent must never suggest raw `gcc` or global Arduino IDE execution steps.
- **Project Context:** The firmware directories (`/sender/` for the traffic light and `/pulsera_main/` for the wristband) must be handled through their respective `platformio.ini` configurations.
- **Dependency Isolation:** All external sensor drivers (e.g., Adafruit MPU6050, QMC5883L Compass, ESP-NOW configurations) must be declared strictly inside the `lib_deps` section of `platformio.ini`. Manual inclusion of external `.h`/`.cpp` library files inside the `src/` folder is prohibited unless they are heavily customized forks.

## 2. Hardware Resource Constraints (ESP32-C3 Focus)
The wristband uses an ESP32-C3 Super Mini. The agent must strictly write code fitting its footprint:
- **Flash & SRAM Optimization:** Avoid excessive logging macros (`Serial.print`) in production states. Wrap non-critical telemetry strings inside the `F()` macro or store them in Flash memory (`PROGMEM`).
- **Compiler Warnings:** Code additions must not trigger compiler warnings. Every function declaration must have strict type definitions, explicit return structures, and clear scope modifiers (`public`, `private`, `protected`).

## 3. Automated Code Quality Gates (PlatformIO Check)
Before presenting any hardware code solution as "complete" or "production-ready," the agent must ensure it passes static analysis.
- **Linting Rule:** Code analysis must be done via PlatformIO's built-in static analysis engine. The agent should utilize the command `pio check` (which runs `cppcheck` under the hood) to catch memory leaks, buffer overflows, or uninitialized pointers.
- **Code Style:** Code blocks must be written in neat, well-commented C++ matching the standard Google/LLVM styling guidelines for embedded environments.

## 4. Centralized Command Runner (`Justfile`)
The root `Justfile` acts as the single source of truth for repository execution. 
- **Enforcement:** If instructing the user to compile, clean, audit, or flash the microcontrollers, the agent **must** refer directly to the native `just <recipe>` command (e.g., `just compile-all`, `just lint-all`, `just flash-pulsera`). Raw terminal commands are restricted.