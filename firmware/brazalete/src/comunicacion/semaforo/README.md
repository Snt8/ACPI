# 📡 src/comunicacion/semaforo
El módulo **src/comunicacion/semaforo** contiene la lógica de la comunicación
del brazalete con el semáforo mediante ESP-NOW.

---

## Cómo recibe
- El callback de ESP-NOW corre en la tarea de WiFi, no en `loop()`. Sólo acepta
  paquetes con el tamaño exacto de `estado_semaforo` y la firma `"ACPI"`
  (`esPaqueteSemaforo()`); los demás se cuentan en `paquetesDescartados()`.
- El paquete válido queda en un buffer protegido con una sección crítica. La FSM lo
  recoge desde `loop()` con `tomarPaquete()`, así toda la lógica corre en un único hilo.
- Sólo se guarda el último paquete: si llegan dos antes de que la FSM lea, el anterior
  ya no sirve.

## Encender y apagar la radio
- `inicializar()` pone el WiFi en modo estación y empieza a escuchar ESP-NOW.
- `desinicializar()` deja de escuchar y apaga el WiFi por completo.
- Ambos son seguros de llamar dos veces seguidas: el modo standby enciende y apaga la
  radio cada 5 s (ver `src/control/modo_standby.h`).
