# Justfile - Automatización Profesional para ACPI (PlatformIO & C++)
.PHONY: compile-all clean-all lint-all flash-pulsera flash-semaforo compile-pulsera compile-semaforo monitor-pulsera monitor-semaforo

# Compila el firmware de la pulsera y del semáforo simultáneamente
compile-all:
    @echo "📦 Compilando firmware de la Pulsera..."
    pio run -d firmware/pulsera
    @echo "📦 Compilando firmware del Semáforo..."
    pio run -d firmware/semaforo

# Compila individualmente la Pulsera
compile-pulsera:
    @echo "📦 Compilando firmware de la Pulsera..."
    pio run -d firmware/pulsera

# Compila individualmente el Semáforo
compile-semaforo:
    @echo "📦 Compilando firmware del Semáforo..."
    pio run -d firmware/semaforo

# Abre el monitor serie de la Pulsera
monitor-pulsera:
    pio device monitor -d firmware/pulsera

# Abre el monitor serie del Semáforo
monitor-semaforo:
    pio device monitor -d firmware/semaforo

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