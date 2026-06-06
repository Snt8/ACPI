#include "fsm.h"
#include "../motores/motor.h"
#include "../brujula/brujula.h"
#include "../comunicacion/semaforo/semaforo.h"
#include "../comunicacion/movil/movil.h"
#include "../utils/calcular_direccion.h"
#include "../utils/patrones_vibracion.h"
#include "../../include/config.h"
#include "../../include/constantes.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

// Definición de variables estáticas
EstadoPulsera MaquinaEstados::estadoActual = EstadoPulsera::BOOTING;
unsigned long MaquinaEstados::ultimoTiempoPaqueteSemaforo = 0;
volatile bool MaquinaEstados::panicoDetectado = false;
unsigned long MaquinaEstados::ultimoTiempoVibracionPanico = 0;
bool MaquinaEstados::motorPanicoEncendido = false;
unsigned long MaquinaEstados::ultimoTiempoEnvioBLE = 0;

// Variables locales para control de vibración en cruce
static unsigned long ultimo_tiempo_derecho = 0;
static bool motor_derecho_encendido = false;
static unsigned long ultimo_tiempo_izquierdo = 0;
static bool motor_izquierdo_encendido = false;

void IRAM_ATTR MaquinaEstados::manejarInterrupcionPanico() {
    // Al presionar el botón de BOOT (activo LOW), alternamos el estado de pánico
    panicoDetectado = !panicoDetectado;
}

void MaquinaEstados::inicializar() {
    // Configurar el botón de pánico
    pinMode(PIN_BOTON_PANICO, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BOTON_PANICO), manejarInterrupcionPanico, FALLING);
    
    ultimoTiempoPaqueteSemaforo = millis();
    ultimoTiempoEnvioBLE = millis();
    
    // Configurar wakeup por GPIO para Light Sleep
    #if defined(ESP_CHIP_ESP32C3) || defined(ARDUINO_ARCH_ESP32)
    esp_sleep_enable_gpio_wakeup();
    gpio_wakeup_enable((gpio_num_t)PIN_BOTON_PANICO, GPIO_INTR_LOW_LEVEL);
    #endif
    
    setEstado(EstadoPulsera::BOOTING);
}

void MaquinaEstados::registrarPaqueteRecibido() {
    ultimoTiempoPaqueteSemaforo = millis();
    if (estadoActual == EstadoPulsera::STANDBY_LOW_POWER) {
        setEstado(EstadoPulsera::SENSING_CROSSING);
    }
}

EstadoPulsera MaquinaEstados::obtenerEstadoActual() {
    return estadoActual;
}

void MaquinaEstados::setEstado(EstadoPulsera nuevoEstado) {
    if (estadoActual == nuevoEstado) return;
    
    Serial.printf("[FSM] Transicion: %d -> %d\n", (int)estadoActual, (int)nuevoEstado);
    
    // Acciones al salir del estado anterior
    if (estadoActual == EstadoPulsera::ALERT_PANIC) {
        ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
    }
    
    estadoActual = nuevoEstado;
    
    // Acciones al entrar al nuevo estado
    if (estadoActual == EstadoPulsera::STANDBY_LOW_POWER) {
        ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
        Serial.println("[FSM] Entrando a modo STANDBY (Bajo Consumo)...");
    }
}

void MaquinaEstados::entrarLightSleep() {
    Serial.println("[FSM] Suspendiendo en Light Sleep...");
    Serial.flush();
    
    // Detener motores
    ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
    
    // Entrar en Light Sleep
    esp_light_sleep_start();
    
    // Al despertar del sleep
    Serial.println("[FSM] Despierto de Light Sleep!");
    ultimoTiempoPaqueteSemaforo = millis(); // Resetear timer
}

void MaquinaEstados::actualizar() {
    unsigned long ahora = millis();

    // Transicionar a PÁNICO desde cualquier estado si la bandera de interrupción está activa
    if (panicoDetectado && estadoActual != EstadoPulsera::ALERT_PANIC) {
        setEstado(EstadoPulsera::ALERT_PANIC);
    } else if (!panicoDetectado && estadoActual == EstadoPulsera::ALERT_PANIC) {
        setEstado(EstadoPulsera::STANDBY_LOW_POWER);
    }

    switch (estadoActual) {
        case EstadoPulsera::BOOTING:
            setEstado(EstadoPulsera::STANDBY_LOW_POWER);
            break;
            
        case EstadoPulsera::STANDBY_LOW_POWER:
            // Si no estamos conectados por BLE y no hemos recibido paquetes en 30 segundos, suspendemos
            if (!ComunicadorMovil::estaConectado() && (ahora - ultimoTiempoPaqueteSemaforo >= 30000UL)) {
                entrarLightSleep();
            }
            break;
            
        case EstadoPulsera::SENSING_CROSSING: {
            // Si no hay paquetes de semáforo en 30s, regresamos a standby
            if (ahora - ultimoTiempoPaqueteSemaforo >= 30000UL) {
                setEstado(EstadoPulsera::STANDBY_LOW_POWER);
                break;
            }
            
            // 1. Calcular heading actual
            ControladorBrujula::obtenerHeading();
            
            // 2. Validar alineación
            bool orientacionOk = ControladorBrujula::revisarSemaforo(ComunicadorSemaforo::ultimoEstado.grados);
            
            // 3. Activar motores según estado y alineación
            if (orientacionOk) {
                if (ComunicadorSemaforo::ultimoEstado.permitidoCruce) {
                    // Alineado y permitido: vibración continua en ambos
                    ControladorMotores::vibrarMotor(ControladorMotores::AMBOS, INTENSIDAD_MOTOR);
                } else {
                    // Alineado pero prohibido: vibración intermitente/espera
                    controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_derecho, motor_derecho_encendido, ControladorMotores::DERECHO, INTENSIDAD_MOTOR);
                    controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_izquierdo, motor_izquierdo_encendido, ControladorMotores::IZQUIERDO, INTENSIDAD_MOTOR);
                }
            } else {
                // Desalineado: detener motores y vibrar según corrección requerida
                ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
                float direccion = calcularDireccion(ComunicadorSemaforo::ultimoEstado.grados, ControladorBrujula::headingActual);
                
                if (direccion < 0) {
                    controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_izquierdo, motor_izquierdo_encendido, ControladorMotores::IZQUIERDO, INTENSIDAD_MOTOR);
                } else if (direccion > 0) {
                    controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_derecho, motor_derecho_encendido, ControladorMotores::DERECHO, INTENSIDAD_MOTOR);
                }
            }
            
            // 4. Reportar telemetría BLE al móvil cada 500ms
            if (ahora - ultimoTiempoEnvioBLE >= 500UL) {
                ultimoTiempoEnvioBLE = ahora;
                
                uint8_t st = 0;
                if (orientacionOk) {
                    st = (ComunicadorSemaforo::ultimoEstado.permitidoCruce) ? 1 : 0;
                } else {
                    st = 2; // orientación incorrecta
                }
                
                ComunicadorMovil::enviarDatos(
                    (uint16_t)ControladorBrujula::headingActual,
                    st,
                    orientacionOk,
                    ComunicadorSemaforo::ultimoEstado.tiempo,
                    -1 // Batería no disponible
                );
            }
            break;
        }
        
        case EstadoPulsera::ALERT_PANIC:
            // Patrón de pánico: vibración intermitente ultra rápida
            controlarVibracion(100, ultimoTiempoVibracionPanico, motorPanicoEncendido, ControladorMotores::AMBOS, INTENSIDAD_MOTOR);
            
            // Enviar estado de pánico (st = 3) al celular cada 500ms
            if (ahora - ultimoTiempoEnvioBLE >= 500UL) {
                ultimoTiempoEnvioBLE = ahora;
                ComunicadorMovil::enviarDatos(
                    (uint16_t)ControladorBrujula::headingActual,
                    3, // Pánico
                    false,
                    0,
                    -1
                );
            }
            break;
    }
}
