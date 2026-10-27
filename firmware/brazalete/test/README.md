# Pruebas del brazalete en la PC

Pruebas unitarias y de escenario de la lógica que **no depende del hardware**: se
compilan y corren en la computadora, sin placa. Arduino IDE y ESP-IDF ignoran esta
carpeta (sólo compilan la raíz del sketch y `src/`).

Para probar el hardware real (sensores, motores) están los sketches de `../pruebas/`.

## Cómo correrlas

Windows, con Visual Studio y su carga de trabajo "Desarrollo de escritorio con C++":
```bat
firmware\brazalete\test\ejecutar_pruebas.bat
```

Linux / macOS / Git Bash con g++ o clang++:
```bash
sh firmware/brazalete/test/ejecutar_pruebas.sh
```

Compilan con todas las advertencias como errores (`/W4 /WX` o `-Wall -Wextra -Werror`),
dejan el ejecutable en `test/build/` (ignorado por git) y terminan con código 1 si
alguna prueba falla.

## Qué se prueba

| Archivo | Cubre |
|---|---|
| `test_controlador_patrones.cpp` | Forma y duración de cada patrón de `patrones.md`, que suene una sola vez, separación mínima de 3 s, que no se solapen, `cancelar()` y el desborde de `millis()`. |
| `test_evaluador_semaforo.cpp` | Qué diferencia entre paquetes es un evento (semáforo nuevo, cambio de luz, otro `id`), el pulso de orientación cada 3 s, el aviso de luz una sola vez, la histéresis de orientación y las lecturas inválidas. |
| `test_escenarios.cpp` | Minutos de uso simulado con el mismo cableado que `fsm.cpp`: sin evento no vibra, cada evento vibra una vez, nunca dos inicios a menos de 3 s, incluso con media hora de entradas al azar. |
| `test_ciclo_escucha.cpp` | Ventanas de escucha del standby: primera escucha inmediata o a los 5 s, duración de la ventana, radio encendida sólo esa fracción del tiempo, despertar tarde sin encadenar ventanas y desborde de `millis()`. |
| `test_utilidades.cpp` | Geometría de orientación (`calcularDireccion`, `desviacionEnfrentamiento`) y el protocolo ESP-NOW (tamaño, firma `"ACPI"`, posición de cada campo). |

`minitest.h` es un mini framework sin dependencias: cada `PRUEBA(nombre)` se registra
sola y `main.cpp` las corre todas.

## Qué no se prueba aquí

Lo que toca hardware o radio (`ControladorMotores`, `ControladorBrujula`, ESP-NOW, BLE,
`ModoStandby` y el light sleep) sólo se compila; de standby se prueba la parte de
tiempos (`CicloEscucha`). Se valida en la placa con los sketches de `pruebas/` y
con el monitor serie del firmware, que imprime cada evento (`[EVAL]`) y cada patrón
iniciado (`[PAT]`).
