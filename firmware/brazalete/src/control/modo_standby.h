#ifndef MODO_STANDBY_H
#define MODO_STANDBY_H

#include <stdint.h>

// ── Modo standby (bajo consumo) ──────────────────────────────────────────────
// Se activa tras TIEMPO_SIN_PAQUETES_STANDBY_MS (30 s) sin paquetes del
// semáforo, y al arrancar. Mientras dura:
//   - no se leen los sensores ni vibran los motores (la FSM no los toca);
//   - el BLE está apagado (la FSM lo apaga antes de llamar a entrar());
//   - el WiFi está apagado, salvo una ventana corta cada 5 s en la que la
//     radio escucha ESP-NOW buscando el paquete "ACPI" del semáforo.
// Entre ventanas la CPU duerme en light sleep si STANDBY_LIGHT_SLEEP vale 1 y
// no hay un cable USB conectado (el light sleep corta el monitor serie).
//
// Los tiempos de las ventanas los decide CicloEscucha; esta clase sólo
// enciende y apaga la radio y duerme.
class ModoStandby {
public:
    // Apaga la radio y programa la primera escucha. Con escucharYa (al
    // arrancar) escucha de inmediato; si no, dentro de 5 s.
    static void entrar(uint32_t ahora, bool escucharYa);

    // Llamar en cada vuelta del loop mientras dure el standby. Devuelve true
    // cuando escuchó un paquete válido del semáforo: en ese caso la radio
    // queda encendida y el paquete sigue en el buffer de ComunicadorSemaforo
    // para que la FSM lo procese. Puede bloquear hasta la próxima ventana.
    static bool actualizar(uint32_t ahora);

private:
    // Espera 'ms' sin hacer nada: light sleep si se puede, delay() si no
    static void esperar(uint32_t ms);
};

#endif // MODO_STANDBY_H
