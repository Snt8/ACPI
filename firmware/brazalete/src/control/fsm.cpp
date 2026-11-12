#include "fsm.h"
#include "coordinador_cruce.h"
#include "modo_standby.h"
#include "../motores/motor.h"
#include "../brujula/brujula.h"
#include "../comunicacion/semaforo/semaforo.h"
#include "../comunicacion/movil/movil.h"
#include "../utils/calcular_direccion.h"
#include "../config.h"
#include "../constantes.h"
#include <Arduino.h>
#include <esp_attr.h>

// ── Definición de variables estáticas ────────────────────────────────────────
EstadoBrazalete MaquinaEstados::estadoActual = EstadoBrazalete::BOOTING;

namespace {

// Lógica de SENSING_CROSSING (paquetes, orientación y patrones). Es la misma
// clase que usan las pruebas de escenario de test/; aquí se le conectan la
// brújula y los motores reales.
CoordinadorCruce cruce;

// Sobrevive a ESP.restart() (no a quitar la batería): marca que el reinicio lo
// pidió la FSM para volver a standby. Al encender trae basura, por eso se
// compara con un valor de 32 bits y no con un bool.
const uint32_t MARCA_REINICIO_STANDBY = 0x53544259u;  // "STBY"
RTC_NOINIT_ATTR uint32_t marca_reinicio;

unsigned long ultimo_envio_ble_ms = 0;

// ── Datos para la telemetría BLE (paquete_telemetria.h) ──────────────────────
uint16_t    secuencia_ble = 0;
Orientacion ultima_orientacion = {false, 0.0f, 0.0f};
bool        hubo_lectura = false;

// Instantes de llegada de los últimos paquetes del semáforo seguido, para
// contar cuántos llegaron en los últimos VENTANA_RECIBIDOS_MS (el semáforo
// manda 2 por segundo: 20 en 10 s es el 100 %)
const uint32_t VENTANA_RECIBIDOS_MS = 10000;
const int      MAX_LLEGADAS = 32;
uint32_t llegadas[MAX_LLEGADAS];
int      n_llegadas = 0;
int      pos_llegadas = 0;

void anotarLlegada(uint32_t ahora) {
    llegadas[pos_llegadas] = ahora;
    pos_llegadas = (pos_llegadas + 1) % MAX_LLEGADAS;
    if (n_llegadas < MAX_LLEGADAS) n_llegadas++;
}

uint8_t llegadasRecientes(uint32_t ahora) {
    int n = 0;
    for (int i = 0; i < n_llegadas; i++) {
        if ((uint32_t)(ahora - llegadas[i]) < VENTANA_RECIBIDOS_MS) n++;
    }
    return (uint8_t)n;
}

// Diagnóstico: rumbo justo antes de cada patrón, para comparar con la primera
// lectura después. Con la persona quieta deberían coincidir; si cambian
// varios grados, la vibración (el imán o el rotor del motor, o la carcasa
// que se movió en la muñeca) está desviando la brújula.
float   rumbo_antes_patron = 0.0f;
bool    rumbo_antes_valido = false;
Patron  patron_diagnostico = Patron::NINGUNO;

const char* nombrePatron(Patron p) {
    switch (p) {
        case Patron::SEMAFORO_DETECTADO: return "SEMAFORO DETECTADO";
        case Patron::GIRAR_DERECHA:      return "GIRAR A LA DERECHA";
        case Patron::GIRAR_IZQUIERDA:    return "GIRAR A LA IZQUIERDA";
        case Patron::CRUCE_PERMITIDO:    return "CRUCE PERMITIDO (verde peatonal)";
        case Patron::CRUCE_PROHIBIDO:    return "CRUCE PROHIBIDO (rojo peatonal)";
        default:                         return "NINGUNO";
    }
}

// ── Orientación ──────────────────────────────────────────────────────────────
// Lee la brújula y calcula qué tan lejos está la persona de quedar de frente
// al cruce y hacia qué lado tiene que girar. El coordinador la llama a 5 Hz.
Orientacion leerOrientacion(const estado_semaforo& semaforo, void*) {
    ControladorBrujula::obtenerHeading();

    Orientacion o;
    o.valida     = ControladorBrujula::headingValido();
    float hp     = ControladorBrujula::headingActual;
    float hs     = (float)semaforo.grados;
    o.desviacion = desviacionEnfrentamiento(hp, hs);
    o.giro       = giroParaEnfrentar(hp, hs);

    Serial.printf("[HDG] Brazalete=%.1f  Semaforo=%d  Desviacion=%.1f  Giro=%.1f%s%s%s\n",
                  hp, (int)hs, o.desviacion, o.giro, o.valida ? "" : "  (lectura invalida: ",
                  ControladorBrujula::motivoInvalida(), o.valida ? "" : ")");
    ultima_orientacion = o;
    hubo_lectura = true;
    return o;
}

// ── Registro en el monitor serie ─────────────────────────────────────────────
void registrarVuelta(const estado_semaforo& paquete, const VueltaCruce& v) {
    if (v.hubo_paquete) {
        switch (v.cambio) {
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
            case CambioPaquete::CAMBIO_POR_CONFIRMAR:
                Serial.println(F("[EVAL] Luz distinta en un paquete: se espera confirmacion"));
                break;
            default:
                break;
        }
    }
    if (v.muestreo) {
        Serial.printf("[EVAL] %s\n", cruce.evaluador().orientado() ? "DE FRENTE" : "MAL ORIENTADO");
        if (patron_diagnostico != Patron::NINGUNO && ControladorBrujula::headingValido()) {
            if (rumbo_antes_valido) {
                float cambio = calcularDireccion(ControladorBrujula::headingActual, rumbo_antes_patron);
                Serial.printf("[DIAG] Tras %s el rumbo cambio %+.1f deg (%.1f -> %.1f)\n",
                              nombrePatron(patron_diagnostico), cambio, rumbo_antes_patron,
                              ControladorBrujula::headingActual);
            }
            patron_diagnostico = Patron::NINGUNO;
        }
    }
    if (v.iniciado != Patron::NINGUNO) {
        Serial.printf("[PAT] %s\n", nombrePatron(v.iniciado));
        rumbo_antes_patron = ControladorBrujula::headingActual;
        rumbo_antes_valido = ControladorBrujula::headingValido();
        patron_diagnostico = v.iniciado;
    }
}

// ── Telemetría BLE ───────────────────────────────────────────────────────────
void enviarTelemetria(uint32_t ahora) {
    const estado_semaforo& s = cruce.evaluador().ultimoPaquete();
    bool orientado = cruce.evaluador().orientado();
    // Sin paquetes recientes la última luz recibida puede ser vieja: no se informa
    bool luz_vigente = cruce.luzVigente(ahora);

    uint8_t st;
    if (!luz_vigente) {
        st = 4;  // Sin señal del semaforo: luz desconocida
    } else if (!orientado) {
        st = 2;  // Orientacion incorrecta
    } else if (s.permitidoCruce) {
        st = 1;  // De frente y verde peatonal: puede cruzar
    } else {
        st = 0;  // De frente y rojo peatonal: debe esperar
    }

    TelemetriaBle t;
    t.secuencia      = secuencia_ble++;
    t.rumbo          = (uint16_t)(lroundf(ControladorBrujula::headingActual) % 360);
    t.estado         = st;
    t.orientado      = orientado;
    t.tiempo         = s.tiempo;
    t.bateria        = -1;  // la PCB v3.5 no mide la batería
    t.semaforo       = s.id;
    t.verde          = s.permitidoCruce != 0;
    t.luz_conocida   = luz_vigente;
    t.rumbo_semaforo = s.grados;
    t.giro           = hubo_lectura ? (int16_t)lroundf(ultima_orientacion.giro) : 0;
    if (!ControladorBrujula::calibrada()) {
        t.brujula = EstadoBrujulaBle::SIN_CALIBRAR;
    } else if (hubo_lectura && ultima_orientacion.valida) {
        t.brujula = EstadoBrujulaBle::VALIDA;
    } else {
        t.brujula = EstadoBrujulaBle::DESCARTADA;
    }
    t.patron    = (uint8_t)cruce.patrones().patronActual();
    t.recibidos = llegadasRecientes(ahora);
    t.rssi      = ComunicadorSemaforo::rssiUltimoPaquete();
    ComunicadorMovil::enviarTelemetria(t);
}

// Apaga los motores y reinicia el ESP32. Con borrar_calibracion, el arranque
// vuelve a calibrar (vibra 1 vez largo al terminar bien, 3 cortas si falla).
// La pausa deja salir la respuesta BLE a la escritura y el monitor serie.
void reiniciarBrazalete(bool borrar_calibracion) {
    cruce.detenerVibracion();
    ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
    if (borrar_calibracion) ControladorBrujula::pedirRecalibracion();
    Serial.flush();
    delay(200);
    ESP.restart();
}

// Comandos de Diagnóstico de la app
void atenderComandoMovil(uint32_t ahora) {
    switch (ComunicadorMovil::tomarComando()) {
        case ComandoMovil::PROBAR_VIBRACION:
            if (cruce.probarVibracion(ahora)) {
                Serial.println(F("[BLE] TEST_ALERT: vibracion de prueba"));
            } else {
                Serial.println(F("[BLE] TEST_ALERT ignorado: los motores no estan libres (3 s entre patrones)"));
            }
            break;
        case ComandoMovil::DETENER_VIBRACION:
            cruce.detenerVibracion();
            Serial.println(F("[BLE] VIBRATE_STOP: patron cortado"));
            break;
        case ComandoMovil::REINICIAR:
            // La calibración de la brújula está en la memoria no volátil: no se pierde
            Serial.println(F("[BLE] RESTART: reiniciando el brazalete..."));
            reiniciarBrazalete(false);
            break;
        case ComandoMovil::RECALIBRAR:
            Serial.println(F("[BLE] RECALIBRATE: reiniciando para calibrar (si falla, se conserva la actual)..."));
            reiniciarBrazalete(true);
            break;
        default:
            break;
    }
}

} // namespace

// ── Inicialización ────────────────────────────────────────────────────────────
void MaquinaEstados::inicializar() {
    estadoActual = EstadoBrazalete::BOOTING;
}

// ── Getters ───────────────────────────────────────────────────────────────────
EstadoBrazalete MaquinaEstados::obtenerEstadoActual() {
    return estadoActual;
}

void MaquinaEstados::reiniciarEnStandby(const char* motivo) {
    Serial.printf("[FSM] Reinicio limpio para volver a standby (%s)\n", motivo);
    cruce.salir();
    ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
    marca_reinicio = MARCA_REINICIO_STANDBY;
    Serial.flush();
    delay(50);
    ESP.restart();
}

bool MaquinaEstados::arranqueTrasStandby() {
    bool fue = (marca_reinicio == MARCA_REINICIO_STANDBY);
    marca_reinicio = 0;
    return fue;
}

const estado_semaforo& MaquinaEstados::ultimoPaquete() {
    return cruce.evaluador().ultimoPaquete();
}

// ── Transición de estado ──────────────────────────────────────────────────────
void MaquinaEstados::setEstado(EstadoBrazalete nuevoEstado) {
    if (estadoActual == nuevoEstado) return;

    Serial.printf("[FSM] Transicion: %d -> %d\n", (int)estadoActual, (int)nuevoEstado);

    EstadoBrazalete anterior = estadoActual;

    // De un cruce a standby: reinicio limpio en vez de apagar el BLE y el WiFi
    // en caliente (ese camino dejaba al brazalete sin volver a encontrar al
    // semáforo). El arranque entra en standby y escucha de inmediato.
    if (anterior == EstadoBrazalete::SENSING_CROSSING &&
        nuevoEstado == EstadoBrazalete::STANDBY_LOW_POWER) {
        reiniciarEnStandby("sin paquetes del semaforo");
    }

    if (anterior == EstadoBrazalete::SENSING_CROSSING) {
        // Se corta cualquier patrón y se olvida el semáforo
        cruce.salir();
        ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
    }

    estadoActual = nuevoEstado;

    if (estadoActual == EstadoBrazalete::STANDBY_LOW_POWER) {
        // Standby: sin BLE, sin WiFi (salvo las ventanas de escucha), sin
        // sensores ni motores. Al arrancar se escucha de inmediato; viniendo
        // de SENSING_CROSSING, tras un intervalo de escucha completo.
        Serial.println(F("[FSM] Entrando a modo STANDBY (Bajo Consumo)..."));
        ComunicadorMovil::desinicializar();
        ControladorBrujula::dormir();
        ModoStandby::entrar((uint32_t)millis(), anterior == EstadoBrazalete::BOOTING);
    }

    if (estadoActual == EstadoBrazalete::SENSING_CROSSING) {
        // La radio ESP-NOW quedó encendida desde la escucha que encontró el
        // semáforo; se despiertan los sensores y se suma el BLE para la
        // telemetría al móvil
        if (!ControladorBrujula::despertar()) {
            Serial.println(F("[FSM] [ERROR] No se pudieron despertar los sensores"));
        }
        if (!ComunicadorMovil::inicializar()) {
            Serial.println(F("[FSM] [ERROR] No se pudo encender el BLE"));
        }
        unsigned long ahora = millis();
        cruce.entrar((uint32_t)ahora);
        ultimo_envio_ble_ms = ahora;
        hubo_lectura = false;
        n_llegadas = 0;
        pos_llegadas = 0;
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
        // ModoStandby enciende la radio unos instantes en cada intervalo de
        // escucha. Cuando oye un paquete del semáforo, éste queda en el buffer
        // para que SENSING_CROSSING lo procese como "semáforo nuevo".
        case EstadoBrazalete::STANDBY_LOW_POWER:
            if (ModoStandby::actualizar((uint32_t)ahora)) {
                setEstado(EstadoBrazalete::SENSING_CROSSING);
            }
            break;

        // ── SENSING_CROSSING ──────────────────────────────────────────────────
        case EstadoBrazalete::SENSING_CROSSING: {
            estado_semaforo paquete{};
            bool hay_paquete = ComunicadorSemaforo::tomarPaquete(paquete);
            atenderComandoMovil((uint32_t)ahora);

            VueltaCruce v = cruce.paso((uint32_t)ahora, hay_paquete ? &paquete : nullptr,
                                       leerOrientacion, nullptr);
            if (v.hubo_paquete && v.cambio != CambioPaquete::IGNORADO) {
                anotarLlegada((uint32_t)ahora);
            }
            registrarVuelta(paquete, v);
            ControladorMotores::aplicar(v.salida.derecho, v.salida.izquierdo);

            // Sin paquetes del semáforo durante TIEMPO_SIN_PAQUETES_STANDBY_MS → standby
            if (cruce.sinPaquetes((uint32_t)ahora)) {
                setEstado(EstadoBrazalete::STANDBY_LOW_POWER);
                break;
            }

            // Telemetría BLE al móvil
            if (ahora - ultimo_envio_ble_ms >= INTERVALO_TELEMETRIA_BLE_MS) {
                ultimo_envio_ble_ms = ahora;
                enviarTelemetria((uint32_t)ahora);
            }
            break;
        }

    }
}
