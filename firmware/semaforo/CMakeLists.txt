# Proyecto ESP-IDF del semáforo ACPI.
# Usa Arduino-ESP32 como componente de ESP-IDF, así semaforo.ino + src/ compilan
# igual que en Arduino IDE. Requiere ESP-IDF v5.5 (la versión que usa Arduino-ESP32 3.3.x).
# El ESP32 DevKit V1 tiene un conversor USB-serie (CP2102/CH340): el Serial va
# por UART0 y no hace falta configurar el USB nativo como en el brazalete.
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(semaforo)
