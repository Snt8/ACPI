//Incluimos las bibliotecas necesarias
#include <Arduino.h>
#include <Wire.h>
//Incluimos los modulos que componen el funcionamiento de la pulsera
#include "../lib/brujula/brujula.h"
#include "../lib/comunicacion/semaforo/semaforo.h"
#include "../lib/utils/calcular_direccion.h"
#include "../lib/utils/patrones_vibracion.h"
#include "../lib/utils/indicar_error.h"
//Definimos constantes globales
#define BAUD_RATE 115200
#define INTENSIDAD_MOTOR 255
#define INTERVALOS_MILISEGUNDOS 500
//Declaramos las variables necesarias durante la ejecucion
unsigned long ultimo_tiempo_derecho = 0;
bool motor_derecho_encendido = false;
unsigned long ultimo_tiempo_izquierdo = 0;
bool motor_izquierdo_encendido = false;
void setup()
{
	//Inicializamos para debug
	Serial.begin(BAUD_RATE);
	//Inicializamos el bus I2C
	Wire.begin();
	//Inicializamos la brujula, los motores y la comunicacion por ESP-Now
	//Si falla algo en el proceso llamamos a la funcion que indica hapticamente el error
	if (!ControladorBrujula::inicializar()) {
		indicarError();
	}
	if (!ControladorMotores::inicializar()) {
		indicarError();
	}
	if (!ComunicadorSemaforo::inicializar()) {
		indicarError();
	}
}

void loop()
{
	//Obtenemos el heading de la brujula
	ControladorBrujula::obtenerHeading();
	//Revisamos que el semaforo este en la direccion correcta
	if (ControladorBrujula::revisarSemaforo(ComunicadorSemaforo::ultimoEstado.grados)) {
		//Si esta en nuestra direccion revisamos si esta en verde
		if (ComunicadorSemaforo::ultimoEstado.permitidoCruce) {
			//Activamos la vibracion de ambos motores para indicar que esta permitido cruzar
			ControladorMotores::vibrarMotor(ControladorMotores::AMBOS, INTENSIDAD_MOTOR);
		}

		//En caso que el semaforo no este en verde
		else {
			//Vibramos alternadamente ambos motores para indicar que el semaforo esta en rojo
			controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_derecho, motor_derecho_encendido, ControladorMotores::DERECHO, INTENSIDAD_MOTOR);
			controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_izquierdo, motor_izquierdo_encendido, ControladorMotores::IZQUIERDO, INTENSIDAD_MOTOR);
		}
	}
	//Si el semaforo no esta en la direccion correcta
	else {
		//Declaramos la direccion para verificar a donde indicar el giro
		float direccion = calcularDireccion(ComunicadorSemaforo::ultimoEstado.grados, ControladorBrujula::headingActual);
		if (direccion < 0) {
			//Activamos el motor izquierdo para indicar que el usuario debe girar hacia ese lado
			controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_izquierdo, motor_izquierdo_encendido, ControladorMotores::IZQUIERDO, INTENSIDAD_MOTOR);
		}
		else if (direccion > 0) {
			//Activamos el motor derecho para indicar que el usuario debe girar hacia ese lado
			controlarVibracion(INTERVALOS_MILISEGUNDOS, ultimo_tiempo_derecho, motor_derecho_encendido, ControladorMotores::DERECHO, INTENSIDAD_MOTOR);
		}
	}
}

