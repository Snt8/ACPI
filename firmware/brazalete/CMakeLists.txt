# Proyecto ESP-IDF del brazalete ACPI.
# Usa Arduino-ESP32 como componente de ESP-IDF, así brazalete.ino + src/ compilan
# igual que en Arduino IDE. Requiere ESP-IDF v5.5 (la versión que usa Arduino-ESP32 3.3.x).
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Serial por el USB nativo del ESP32-C3 (equivale a "USB CDC On Boot: Enabled")
idf_build_set_property(COMPILE_DEFINITIONS "ARDUINO_USB_MODE=1" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "ARDUINO_USB_CDC_ON_BOOT=1" APPEND)

project(brazalete)
