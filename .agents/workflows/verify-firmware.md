---
name: verify-firmware
trigger: /verify
description: Automatiza la compilación, análisis estático (linter) y verificación de calidad de la Pulsera y el Semáforo usando PlatformIO y el Justfile.
---

# Workflow: Automated Firmware Verification & Code Quality Gate

## Metadata
- **Trigger:** `/verify`
- **Description:** Automatically orchestrates the compilation, static analysis (linter), and verification of the ACPI firmware modules (Pulsera and Semáforo) utilizing the root Justfile and PlatformIO CLI tools.

## 1. Workflow Execution Steps
When the user invokes the `/verify` command, the agent must execute the following sequence autonomously:

1. **Environment Initialization:** Check if the workspace contains the root `Justfile` and valid `platformio.ini` profiles inside `/sender/` and `/pulsera_main/`.
2. **Static Analysis (Linting Gate):**
   - Execute the shell task: `just lint-all`
   - Capture the output streams from `pio check` / `cppcheck`.
3. **Firmware Compilation (Build Gate):**
   - Execute the shell task: `just compile-all`
   - Track build progress for both target chips (ESP32 DevKit V1 and ESP32-C3).

## 2. Response and Failure Handling
The agent must process the terminal results and present them using a highly structured, professional developer format:

- **If everything passes successfully:**
  Output a clean report using green indicator flags, summary of memory utilization (RAM/Flash percentages reported by PlatformIO), and a clear statement that the code is ready for flashing via `just flash-pulsera` or `just flash-semaforo`.

- **If the Linter or Compiler fails:**
  - Do not spit raw unformatted logs. 
  - Parse the terminal errors and extract: **File Name**, **Line Number**, **Error Type (Warning/Defect/Error)**, and a **Brief Suggestion** on how to fix it.
  - Automatically offer a multi-choice prompt to the user to fix the errors autonomously (e.g., *"Would you like me to refactor the memory assignment on Line X to fix this warning?"*).

## 3. UI Display Blueprint
```text
⚙️ ACPI Verification Report:
┌──────────────────────────────┬──────────┬─────────────┐
│ Module                       │ Build    │ Static Lint │
├──────────────────────────────┼──────────┼─────────────┤
│ Pulsera (ESP32-C3)           │ [PASS]   │ [PASS]      │
│ Semáforo (ESP32 DevKit)      │ [PASS]   │ [1 Warning] │
└──────────────────────────────┴──────────┴─────────────┘
📊 Memory Footprint (Pulsera): RAM: 14.2% | Flash: 42.1%