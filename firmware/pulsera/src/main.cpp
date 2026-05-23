//Incluimos las bibliotecas necesarias
#include <Arduino.h>
#include <Wire.h>
//Incluimos los modulos que componen el funcionamiento de la pulsera

//Brujula y direccion
#include "../lib/brujula/brujula.h"
#include "../lib/utils/calcular_direccion.h"
//Comunicacion semaforo y celular(proximamente)
#include "../lib/comunicacion/semaforo/semaforo.h"
//Motores y feedback haptico
#include "../lib/utils/patrones_vibracion.h"
#include "../lib/utils/indicar_error.h"
//Parametros y constantes de configuracoion
#include "../include/config.h"
#include "../include/constantes.h"
//Declaramos las variables necesarias durante la ejecucion
unsigned long ultimo_tiempo_derecho = 0;
bool motor_derecho_encendido = false;
unsigned long ultimo_tiempo_izquierdo = 0;
bool motor_izquierdo_encendido = false;

namespace {
    const unsigned long TIEMPO_ESPERA_SERIAL_MS = 1500;
    const unsigned long INTERVALO_HEARTBEAT_MS = 1000;
}

unsigned long ultimo_tiempo_heartbeat = 0;

void setup()
{
	//Inicializamos para debug
	Serial.begin(BAUD_RATE);

	//Damos un margen corto para que el monitor serial alcance a mostrar los primeros mensajes
	unsigned long inicio_espera_serial = millis();
	//while (!Serial && (millis() - inicio_espera_serial < TIEMPO_ESPERA_SERIAL_MS)) {
		//delay(10);
	//}

  delay(200);
  Serial.println();
  Serial.println("[BOOT] Iniciando pulsera...");
  Serial.print("[BOOT] BAUD_RATE = ");
  Serial.println(BAUD_RATE);

	//Inicializamos el bus I2C
	Wire.begin();
	//Inicializamos la brujula, los motores y la comunicacion por ESP-Now
	//Si falla algo en el proceso llamamos a la funcion que indica hapticamente el error
    Serial.println("[BOOT] Inicializando brujula...");
    if (!ControladorBrujula::inicializar()) {
        Serial.println("[ERROR] Fallo al inicializar la brujula");
        Serial.flush();
        indicarError();
    }
    Serial.println("[BOOT] Brujula lista");

    Serial.println("[BOOT] Inicializando motores...");
    if (!ControladorMotores::inicializar()) {
        Serial.println("[ERROR] Fallo al inicializar los motores");
        Serial.flush();
        indicarError();
    }
    Serial.println("[BOOT] Motores listos");

    Serial.println("[BOOT] Inicializando comunicacion ESP-NOW...");
    if (!ComunicadorSemaforo::inicializar()) {
        Serial.println("[ERROR] Fallo al inicializar la comunicacion con semaforo");
        Serial.flush();
        indicarError();
    }

    Serial.println("[BOOT] Sistema listo");
}

// Variables globales para control de tiempos (fuera del loop)
unsigned long ultimo_tiempo_print = 0;
const unsigned long intervalo_print = 200; // Imprime cada 200ms

void loop()
{
    unsigned long tiempo_actual = millis();

    if (tiempo_actual - ultimo_tiempo_heartbeat >= INTERVALO_HEARTBEAT_MS) {
        ultimo_tiempo_heartbeat = tiempo_actual;
        Serial.print("[LOOP] Vivo | ms=");
        Serial.println(tiempo_actual);
    }

    // 1. Obtener datos de sensores
    ControladorBrujula::obtenerHeading();

    // 2. Lógica de control (Motores y semáforo)
    // Asegúrate de que controlarVibracion() también use millis() internamente
    if (ControladorBrujula::revisarSemaforo(ComunicadorSemaforo::ultimoEstado.grados)) {
        if (ComunicadorSemaforo::ultimoEstado.permitidoCruce) {
            ControladorMotores::vibrarMotor(ControladorMotores::AMBOS, INTENSIDAD_MOTOR);
        } else {
            controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_derecho, motor_derecho_encendido, ControladorMotores::DERECHO, INTENSIDAD_MOTOR);
            controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_izquierdo, motor_izquierdo_encendido, ControladorMotores::IZQUIERDO, INTENSIDAD_MOTOR);
        }
    } else {
        ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
        float direccion = calcularDireccion(ComunicadorSemaforo::ultimoEstado.grados, ControladorBrujula::headingActual);

        if (direccion < 0) {
            controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_izquierdo, motor_izquierdo_encendido, ControladorMotores::IZQUIERDO, INTENSIDAD_MOTOR);
        } else if (direccion > 0) {
            controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_derecho, motor_derecho_encendido, ControladorMotores::DERECHO, INTENSIDAD_MOTOR);
        }
    }

    // 3. Impresión no bloqueante (Solo ocurre cada 200ms)
    if (tiempo_actual - ultimo_tiempo_print >= intervalo_print) {
        ultimo_tiempo_print = tiempo_actual;

        float diferencia = calcularDireccion(ComunicadorSemaforo::ultimoEstado.grados, ControladorBrujula::headingActual);

        Serial.print("[BRUJULA] Heading: ");
        Serial.print(ControladorBrujula::headingActual, 2);
        Serial.print(" | [SEMAFORO] Grados: ");
        Serial.print(ComunicadorSemaforo::ultimoEstado.grados);
        Serial.print(" | Permitido: ");
        Serial.print(ComunicadorSemaforo::ultimoEstado.permitidoCruce ? "SI" : "NO");
        Serial.print(" | Delta: ");
        Serial.println(diferencia, 2);
    }
}

