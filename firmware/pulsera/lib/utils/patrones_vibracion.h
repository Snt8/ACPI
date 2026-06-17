#ifndef PULSERA_PATRONES_VIBRACION_H
#define PULSERA_PATRONES_VIBRACION_H

#include "../motores/motor.h"

// ── Patrones de vibración no-bloqueantes ──────────────────────────────────────
// Todas las funciones usan millis() internamente.
// IMPORTANTE: deben llamarse en cada iteración del loop() (no solo cada 500 ms)
// para que la maquina de estados interna avance con los tiempos correctos.

// Patrón ROJO (cruce autorizado, ambos motores):
//   Ciclo: 2 pulsos de 150ms con pausa 200ms → silencio 700ms → repite (~1.2s)
void vibracionConfirmatoria(ControladorMotores::Motor motor);

// Patrón VERDE (semaforo en verde, esperar, ambos motores):
//   3 pulsos por grupo × 5 grupos → silencio 1500ms → repite
void vibracionVerde(ControladorMotores::Motor motor);

// Patrón DIRECCIONAL (usuario mal orientado):
//   Ciclo: pulso 200ms → pausa 700ms → repite (~1 Hz)
void vibracionDireccional(ControladorMotores::Motor motor);

// Reinicia el estado interno de todos los patrones y apaga ambos motores.
// Llamar al cambiar de estado en la FSM o al cambiar de patron.
void reiniciarPatrones();

#endif // PULSERA_PATRONES_VIBRACION_H
