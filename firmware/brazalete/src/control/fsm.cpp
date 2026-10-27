#include "fsm.h"
#include "evaluador_semaforo.h"
#include "modo_standby.h"
#include "../vibracion/controlador_patrones.h"
#include "../motores/motor.h"
#include "../brujula/brujula.h"
#include "../comunicacion/semaforo/semaforo.h"
#include "../comunicacion/movil/movil.h"
#include "../utils/calcular_direccion.h"
#include "../config.h"
#include "../constantes.h"
#include <Arduino.h>

// ── Definición de variables estáticas ────────────────────────────────────────
EstadoBrazalete  MaquinaEstados::estadoActual                = EstadoBrazalete::BOOTING;
unsigned long  MaquinaEstados::ultimoTiempoPaqueteSemaforo = 0;
unsigned long  MaquinaEstados::ultimoTiempoEnvioBLE        = 0;

// ── Vibración ────────────────────────────────────────────────────────────────
// El controlador reproduce los patrones y el evaluador decide cuáles, a partir
// de las diferencias entre paquetes y de la orientación. La FSM es la única
// que escribe en los motores, con lo que devuelve el controlador.
static ControladorPatrones patrones;
static EvaluadorSemaforo   evaluador(patrones);

// ── Variables locales del estado SENSING_CROSSING ────────────────────────────
static unsigned long ultimo_muestreo_ms = 0;

static const char* nombrePatron(Patron p) {
    switch (p) {
        case Patron::SEMAFORO_DETECTADO: return "SEMAFORO DETECTADO";
        case Patron::GIRAR_DERECHA:      return "GIRAR A LA DERECHA";
        case Patron::GIRAR_IZQUIERDA:    return "GIRAR A LA IZQUIERDA";
        case Patron::CRUCE_PERMITIDO:    return "CRUCE PERMITIDO (verde peatonal)";
        case Patron::CRUCE_PROHIBIDO:    return "CRUCE PROHIBIDO (rojo peatonal)";
        default:                         return "NINGUNO";
    }
}

// ── Paquetes del semáforo ────────────────────────────────────────────────────
// Recoge el paquete que dejó el callback de ESP-NOW y se lo pasa al evaluador.
// Sólo los paquetes del semáforo que se está siguiendo mantienen vivo el
// temporizador de standby.
static void procesarPaquetes(unsigned long ahora, unsigned long& ultimoPaquete) {
    estado_semaforo paquete;
    if (!ComunicadorSemaforo::tomarPaquete(paquete)) return;

    CambioPaquete cambio = evaluador.registrarPaquete(paquete);
    if (cambio == CambioPaquete::IGNORADO) return;
    ultimoPaquete = ahora;

    switch (cambio) {
        case CambioPaquete::SEMAFORO_NUEVO:
            Serial.printf("[EVAL] Semaforo %u detectado (grados=%u, cruce=%s)\n",
                          paquete.id, paquete.grados, paquete.permitidoCruce ? "SI" : "NO");
            break;
        case CambioPaquete::CAMBIO_A_PERMITIDO:
            Serial.println(F("[EVAL] Cambio de luz: cruce PERMITIDO"));
            break;
        case CambioPaquete::CAMBIO_A_PROHIBIDO:
            Serial.println(F("[EVAL] Cambio de luz: cruce PROHIBIDO"));
            break;
        default:
            break;
    }
}

// ── Orientación ──────────────────────────────────────────────────────────────
// Lee la brújula y le pasa al evaluador qué tan lejos está la persona de
// quedar de frente al cruce y hacia qué lado tiene que girar.
static void muestrearOrientacion() {
    ControladorBrujula::obtenerHeading();

    Orientacion o;
    o.valida = ControladorBrujula::headingValido();
    float hp = ControladorBrujula::headingActual;
    float hs = (float)evaluador.ultimoPaquete().grados;
    o.desviacion = desviacionEnfrentamiento(hp, hs);
    // De frente al semáforo = mirando hacia hs + 180°
    o.giro = calcularDireccion(fmodf(hs + 180.0f, GRADOS_CIRCUNFERENCIA), hp);
    evaluador.registrarOrientacion(o);

    Serial.printf("[HDG] Brazalete=%.1f  Semaforo=%d  Desviacion=%.1f  Giro=%.1f  %s%s\n",
                  hp, (int)hs, o.desviacion, o.giro,
                  evaluador.orientado() ? "DE FRENTE" : "MAL ORIENTADO",
                  o.valida ? "" : "  (lectura invalida)");
}

// ── Telemetría BLE ───────────────────────────────────────────────────────────
static void enviarTelemetria() {
    const estado_semaforo& s = evaluador.ultimoPaquete();
    bool orientado = evaluador.orientado();

    uint8_t st;
    if (!orientado) {
        st = 2;  // Orientacion incorrecta
    } else if (s.permitidoCruce) {
        st = 1;  // Cruce permitido: orientado, semaforo rojo para autos (verde peaton)
    } else {
        st = 0;  // Cruce prohibido: orientado pero semaforo verde para autos (rojo peaton)
    }

    ComunicadorMovil::enviarDatos(
        (uint16_t)ControladorBrujula::headingActual,
        st,
        orientado,
        s.tiempo,
        -1  // Bateria no disponible
    );
}

// ── Inicialización ────────────────────────────────────────────────────────────
void MaquinaEstados::inicializar() {
    ultimoTiempoPaqueteSemaforo = millis();
    ultimoTiempoEnvioBLE        = millis();
    ultimo_muestreo_ms          = millis();

    setEstado(EstadoBrazalete::BOOTING);
}

// ── Getter de estado actual ───────────────────────────────────────────────────
EstadoBrazalete MaquinaEstados::obtenerEstadoActual() {
    return estadoActual;
}

// ── Transición de estado ──────────────────────────────────────────────────────
void MaquinaEstados::setEstado(EstadoBrazalete nuevoEstado) {
    if (estadoActual == nuevoEstado) return;

    Serial.printf("[FSM] Transicion: %d -> %d\n", (int)estadoActual, (int)nuevoEstado);

    EstadoBrazalete anterior = estadoActual;

    if (anterior == EstadoBrazalete::SENSING_CROSSING) {
        // Se corta cualquier patrón y se olvida el semáforo. El controlador
        // conserva la hora del último patrón: la separación mínima se respeta
        // aunque el brazalete vuelva a encontrar el semáforo enseguida.
        patrones.cancelar();
        ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
        evaluador.reiniciar();
    }

    estadoActual = nuevoEstado;

    if (estadoActual == EstadoBrazalete::STANDBY_LOW_POWER) {
        // Standby: sin BLE, sin WiFi (salvo las ventanas de escucha), sin
        // sensores ni motores. Al arrancar se escucha de inmediato; viniendo
        // de SENSING_CROSSING, a los 5 s.
        Serial.println(F("[FSM] Entrando a modo STANDBY (Bajo Consumo)..."));
        ComunicadorMovil::desinicializar();
        ModoStandby::entrar((uint32_t)millis(), anterior == EstadoBrazalete::BOOTING);
    }

    if (estadoActual == EstadoBrazalete::SENSING_CROSSING) {
        // La radio ESP-NOW quedó encendida desde la escucha que encontró el
        // semáforo; se suma el BLE para la telemetría al móvil
        if (!ComunicadorMovil::inicializar()) {
            Serial.println(F("[FSM] [ERROR] No se pudo encender el BLE"));
        }
        unsigned long ahora = millis();
        ultimoTiempoPaqueteSemaforo = ahora;
        ultimoTiempoEnvioBLE        = ahora;
        ultimo_muestreo_ms          = ahora;
    }
}

// ── Loop principal ────────────────────────────────────────────────────────────
void MaquinaEstados::actualizar() {
    unsigned long ahora = millis();

    switch (estadoActual) {

        // ── BOOTING ───────────────────────────────────────────────────────────
        case EstadoBrazalete::BOOTING:
            setEstado(EstadoBrazalete::STANDBY_LOW_POWER);
            break;

        // ── STANDBY_LOW_POWER ─────────────────────────────────────────────────
        // ModoStandby enciende la radio unos instantes cada 5 s. Cuando oye un
        // paquete del semáforo, éste queda en el buffer para que
        // SENSING_CROSSING lo procese como "semáforo nuevo".
        case EstadoBrazalete::STANDBY_LOW_POWER:
            if (ModoStandby::actualizar((uint32_t)ahora)) {
                setEstado(EstadoBrazalete::SENSING_CROSSING);
            }
            break;

        // ── SENSING_CROSSING ──────────────────────────────────────────────────
        case EstadoBrazalete::SENSING_CROSSING: {
            procesarPaquetes(ahora, ultimoTiempoPaqueteSemaforo);

            // Timeout: sin paquetes del semáforo → volver a standby
            if (ahora - ultimoTiempoPaqueteSemaforo >= TIEMPO_SIN_PAQUETES_STANDBY_MS) {
                setEstado(EstadoBrazalete::STANDBY_LOW_POWER);
                break;
            }

            // Muestreo a 2 Hz. Mientras vibra un motor no se lee la brújula:
            // los imanes de los motores alteran el magnetómetro.
            if (ahora - ultimo_muestreo_ms >= INTERVALO_MUESTREO_MS && !patrones.sonando()) {
                ultimo_muestreo_ms = ahora;
                muestrearOrientacion();
            }

            // El evaluador inicia un patrón sólo si hay un evento y el
            // controlador está libre (separación mínima de 3 s)
            Patron iniciado = evaluador.orquestar((uint32_t)ahora);
            if (iniciado != Patron::NINGUNO) {
                Serial.printf("[PAT] %s\n", nombrePatron(iniciado));
            }

            // Los motores siguen exactamente lo que dice el controlador
            SalidaMotores salida = patrones.actualizar((uint32_t)ahora);
            ControladorMotores::aplicar(salida.derecho, salida.izquierdo);

            // Telemetría BLE al móvil
            if (ahora - ultimoTiempoEnvioBLE >= 500UL) {
                ultimoTiempoEnvioBLE = ahora;
                enviarTelemetria();
            }
            break;
        }

    }
}
