# Justfile - Automatización Profesional para ACPI (PlatformIO & C++)
.PHONY: compile-all clean-all lint-all flash-pulsera flash-semaforo

# Compila el firmware de la pulsera y del semáforo simultáneamente
compile-all:
    @echo "📦 Compilando firmware de la Pulsera..."
    pio run -d firmware/pulsera
    @echo "📦 Compilando firmware del Semáforo..."
    pio run -d firmware/semaforo

# Limpia los archivos temporales de compilación de PlatformIO
clean-all:
    @echo "🧹 Limpiando entornos..."
    pio run -d firmware/pulsera --target clean
    pio run -d firmware/semaforo --target clean

# Realiza análisis estático de código (Linter integrado de PlatformIO/Cppcheck)
lint-all:
    @echo "🔍 Analizando código de la Pulsera..."
    pio check -d firmware/pulsera --fail-on-defect=low
    @echo "🔍 Analizando código del Semáforo..."
    pio check -d firmware/semaforo --fail-on-defect=low

# Flashea el firmware a la pulsera (ESP32-C3)
flash-pulsera:
    pio run -d firmware/pulsera --target upload

# Flashea el firmware al semáforo (ESP32 DevKit V1)
flash-semaforo:
    pio run -d firmware/semaforo --target upload