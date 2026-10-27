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

Windows con el g++ de WSL (así se corrieron en esta PC), desde la raíz del repo:
```bash
wsl sh firmware/brazalete/test/ejecutar_pruebas.sh
```

Compilan con todas las advertencias como errores (`/W4 /WX` o `-Wall -Wextra -Werror`),
dejan el ejecutable en `test/build/` (ignorado por git) y terminan con código 1 si
alguna prueba falla.

## Qué se prueba

| Archivo | Cubre |
|---|---|
| `test_controlador_patrones.cpp` | Forma y duración de cada patrón de `patrones.md`, que suene una sola vez, separación mínima de 3 s, descanso de 1 s tras cada patrón, que no se solapen, `cancelar()` y el desborde de `millis()`. |
| `test_evaluador_semaforo.cpp` | Qué diferencia entre paquetes es un evento (semáforo nuevo, cambio de luz, otro `id`), el pulso de orientación cada 3 s (también tras el desborde de `millis()`), el aviso de luz una sola vez y su espera si llega durante otro aviso, la confirmación del cambio de luz con dos paquetes seguidos (un paquete suelto no hace vibrar), la histéresis de orientación, las lecturas inválidas y el lado del giro en los empates. |
| `test_escenarios.cpp` | Minutos de uso simulado con el mismo `CoordinadorCruce` que usa `fsm.cpp` (sólo se simulan los paquetes y la brújula): sin evento no vibra, cada evento vibra una vez, los paquetes sueltos con otra luz no hacen vibrar, nunca dos inicios a menos de 3 s ni sin el descanso de 1 s, paso a standby a los 30 s sin paquetes del semáforo seguido (los de otro `id` no cuentan), incluso con media hora de entradas y cortes de señal al azar. |
| `test_ciclo_escucha.cpp` | Ventanas de escucha del standby: primera escucha inmediata o a los 5 s, duración de la ventana, radio encendida sólo esa fracción del tiempo, despertar tarde sin encadenar ventanas y desborde de `millis()`. |
| `test_utilidades.cpp` | Geometría de orientación (`calcularDireccion`, `desviacionEnfrentamiento`) y el protocolo ESP-NOW (tamaño, firma `"ACPI"`, CRC-8 ante cualquier bit alterado y ráfagas de ruido, rangos de los campos, posición de cada campo). |

`minitest.h` es un mini framework sin dependencias: cada `PRUEBA(nombre)` se registra
sola y `main.cpp` las corre todas.

## Qué no se prueba aquí

Lo que toca hardware o radio (`ControladorMotores`, `ControladorBrujula`, ESP-NOW, BLE,
`ModoStandby` y el light sleep) sólo se compila; de standby se prueba la parte de
tiempos (`CicloEscucha`) y, en los escenarios, la transición por falta de paquetes.
Eso se valida en la placa con los sketches de `pruebas/` y con el monitor serie del
firmware, que imprime cada evento (`[EVAL]`), cada patrón iniciado (`[PAT]`) y, en el
heartbeat, el heap libre (para ver si los ciclos de standby pierden memoria).

Un límite conocido: `INTERVALO_GIRO_MS` y `SEPARACION_MINIMA_EVENTOS_MS` valen lo mismo
(3 s), así que ninguna prueba distingue cuál de los dos frena el pulso de orientación.
Ambos frenos existen; si algún día se separan los valores, conviene agregar esa prueba.
