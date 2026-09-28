#ifndef CONFIG_H
#define CONFIG_H

// ── Serial ────────────────────────────────────────────────────────────────────
#define BAUD_RATE 115200

// ── Lógica de comparación diferencial ────────────────────────────────────────
// Condición de cruce seguro: |Δθ - 180°| < UMBRAL_CRUCE_GRADOS
// donde Δθ = min(|Hp - Hs|, 360 - |Hp - Hs|)
// Los nodos se ENFRENTAN cuando el usuario está correctamente orientado para cruzar.
#define UMBRAL_CRUCE_GRADOS     30.0f

// ── Timing de vibración de error (indicar_error.cpp) ─────────────────────────
#define INTERVALOS_ERROR        200

#endif // CONFIG_H