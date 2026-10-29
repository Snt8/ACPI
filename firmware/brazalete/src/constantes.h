#ifndef CONSTANTES_H
#define CONSTANTES_H

#include <math.h>

// ── Matemáticas circulares ────────────────────────────────────────────────────
#define GRADOS_CIRCUNFERENCIA   360.0f
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define RAD_A_GRADOS            (180.0f / (float)M_PI)
#define GRADOS_A_RAD            ((float)M_PI / 180.0f)

// ── Motores ───────────────────────────────────────────────────────────────────
#define APAGAR_MOTOR            0
// En la PCB v3.5 el motor ve VBAT (3.7–4.2 V): a 180/255 vibraba tan fuerte que
// amenazaba las soldaduras, y 120 seguía siendo mucho. 100/255 ≈ 39 % de duty.
// Si a este valor un motor no llega a arrancar, verlo con el barrido 'b' de la
// prueba 03, que muestra desde qué duty arranca cada uno.
#define INTENSIDAD_MOTOR        100
// Motor izquierdo (GPIO 10). Antes llevaba +15 para igualar la sensación, un
// valor heredado que nunca se confirmó en esta placa: queda igual al derecho
// hasta medirlo con los dos motores soldados.
#define INTENSIDAD_MOTOR_IZQUIERDO 100

// ── Acelerómetro MPU6050 (rango ±2g por defecto) ────────────────────────────
// Datasheet: 16384 LSB/g en rango ±2g.
// NOTA: El requisito original indicaba 16600.0, pero el valor correcto del
// datasheet del MPU6050 es 16384.0 LSB/g. Se usa el valor del datasheet.
#define FACTOR_ESCALA_ACCEL     16384.0f

// ── Calibración del magnetómetro (ver brujula/calibracion_magnetica.h) ──────
// Cada eje tiene que recorrer al menos este rango (máx − mín) para dar la
// calibración por buena. El QMC5883L a ±8 G da 3000 LSB/G; el campo de la
// Tierra en Colombia es de ~0.33 G (~1000 LSB), así que al girar el brazalete
// en todas direcciones cada eje recorre ~2000 LSB. 1200 LSB exige ~60 %.
#define CALIBRACION_SPAN_MINIMO_LSB    1200.0f
// La calibración termina cuando está lista y lleva este tiempo sin crecer
// más de un 1 %, o al llegar al tiempo máximo (entonces queda sin calibrar)
#define CALIBRACION_ESTABLE_MS         5000UL
#define CALIBRACION_DURACION_MAX_MS    90000UL
#define CALIBRACION_INTERVALO_MS       50UL

// ── Lectura de la brújula ────────────────────────────────────────────────────
// Cada lectura promedia estas muestras de ambos sensores (el QMC5883L mide a
// 200 Hz: una muestra nueva cada 5 ms). Reduce el ruido del rumbo a la mitad.
#define MUESTRAS_POR_LECTURA           4
#define PAUSA_ENTRE_MUESTRAS_MS        5
// Si el acelerómetro no mide ~1 g, la muñeca se está moviendo y la gravedad
// medida no sirve para compensar la inclinación: la lectura se descarta
#define TOLERANCIA_GRAVEDAD_G          0.15f

// ── Frecuencia de muestreo de la brújula (5 Hz) ──────────────────────────────
// Con el cono de 5° la lectura tiene que seguir a la persona mientras gira: a
// 2 Hz, girando a 10°/s, entre dos lecturas avanzaba 5° (medio cono) y el
// aviso llegaba tarde. Cada lectura bloquea ~15 ms (4 muestras).
#define INTERVALO_MUESTREO_MS   200UL

// Tras cada patrón se espera esto antes de volver a leer la brújula: el motor
// sigue girando por inercia y la muñeca sigue vibrando un instante después de
// cortar la corriente, y esa lectura podía sacar a la persona de "de frente".
#define ASENTAMIENTO_TRAS_VIBRAR_MS 300UL

// ── Telemetría BLE al móvil ──────────────────────────────────────────────────
#define INTERVALO_TELEMETRIA_BLE_MS 500UL

// ── Safety check de heading (brazalete) ────────────────────────────────────────
// Si el heading cambia más de UMBRAL_DELTA_HEADING entre dos lecturas separadas
// por menos de UMBRAL_TIEMPO_SAFETY_MS, la lectura se invalida (giro brusco de
// la muñeca) y no se indica ningún giro. El umbral de tiempo tiene que ser MAYOR
// que el intervalo de muestreo: con los dos en 500 ms, dos lecturas seguidas
// nunca estaban a menos de 500 ms y el chequeo no se activaba nunca.
// Tras una pausa larga (mientras vibra un motor no se lee la brújula) la primera
// lectura no se compara: la referencia es demasiado vieja.
#define UMBRAL_DELTA_HEADING    40.0f
#define UMBRAL_TIEMPO_SAFETY_MS (2 * INTERVALO_MUESTREO_MS)

static_assert(UMBRAL_TIEMPO_SAFETY_MS > INTERVALO_MUESTREO_MS,
              "El safety check debe abarcar dos muestras consecutivas");

// ── Standby ──────────────────────────────────────────────────────────────────
// Sin paquetes del semáforo durante este tiempo, el brazalete pasa a standby.
// El semáforo envía un paquete cada 500 ms y cambia de luz cada 25 s.
#define TIEMPO_SIN_PAQUETES_STANDBY_MS 30000UL

// En standby la radio sólo escucha una ventana corta cada INTERVALO (inicio a
// inicio); el resto del tiempo el WiFi está apagado. La ventana abarca más de
// dos envíos del semáforo: si el WiFi tarda menos de 200 ms en arrancar,
// escucha al menos dos paquetes.
#define INTERVALO_ESCUCHA_STANDBY_MS   5000UL
#define VENTANA_ESCUCHA_STANDBY_MS     1200UL
#define INTERVALO_ENVIO_SEMAFORO_MS    500UL   // INTERVALO_ENVIO_MS en firmware/semaforo

static_assert(VENTANA_ESCUCHA_STANDBY_MS > 2 * INTERVALO_ENVIO_SEMAFORO_MS,
              "La ventana de escucha debe abarcar al menos dos envios del semaforo");
static_assert(VENTANA_ESCUCHA_STANDBY_MS < INTERVALO_ESCUCHA_STANDBY_MS,
              "La ventana de escucha debe ser mas corta que el intervalo");

// ── Watchdog del loop ────────────────────────────────────────────────────────
// Si loop() deja de volver durante este tiempo (algo se quedó pegado: un
// periférico que no responde, una radio saturada), el ESP32 se reinicia solo y
// arranca en standby con los motores apagados. Es el timeout del task watchdog
// del core (CONFIG_ESP_TASK_WDT_TIMEOUT_S = 5). Nada del firmware puede
// bloquear tanto: lo más largo es el light sleep entre escuchas.
#define TIMEOUT_WATCHDOG_LOOP_MS       5000UL

static_assert(INTERVALO_ESCUCHA_STANDBY_MS - VENTANA_ESCUCHA_STANDBY_MS < TIMEOUT_WATCHDOG_LOOP_MS,
              "El light sleep del standby no puede durar mas que el watchdog");

// ── Monitor de interferencia ─────────────────────────────────────────────────
// Si en un heartbeat (2 s) llegan más paquetes ESP-NOW ajenos que este umbral,
// se avisa por el monitor serie. Sólo informa: esos paquetes ya se descartan.
#define UMBRAL_INTERFERENCIA_PAQUETES  20

// ── Patrones de vibración (ver patrones.md) ──────────────────────────────────
// Cada patrón suena UNA vez por evento y nunca se repite solo. "Pulso" = motor
// encendido a su INTENSIDAD_*; "pausa" = motor apagado entre dos pulsos.

// Regla de protección de las soldaduras: entre el inicio de un patrón y el
// del siguiente pasan al menos estos ms, y dos patrones nunca se solapan.
// La garantiza ControladorPatrones; ningún patrón puede saltársela.
#define SEPARACION_MINIMA_EVENTOS_MS   3000UL

// Además, tras terminar un patrón los motores descansan al menos este tiempo
// antes del siguiente. Importa con los avisos de luz (4.5 s y 2 s): sin esto,
// un cambio de luz durante el verde haría sonar el rojo pegado a él.
#define SILENCIO_TRAS_PATRON_MS        1000UL

// 1. Semáforo detectado: 3 pulsos cortos de ambos motores (1.6 s en total)
#define DETECTADO_PULSOS               3
#define DETECTADO_PULSO_MS             200UL
#define DETECTADO_PAUSA_MS             500UL

// 2. Orientación: 1 pulso del motor del lado hacia el que hay que girar.
//    Se repite cada INTERVALO_GIRO_MS mientras la persona siga mal orientada.
#define GIRO_PULSO_MS                  300UL
#define INTERVALO_GIRO_MS              3000UL

// 3. Cruce permitido (semáforo peatonal en verde): 2 pulsos sostenidos (4.5 s)
#define PERMITIDO_PULSOS               2
#define PERMITIDO_PULSO_MS             2000UL
#define PERMITIDO_PAUSA_MS             500UL

// 4. Cruce prohibido (semáforo peatonal en rojo): 1 pulso sostenido (2 s)
#define PROHIBIDO_PULSOS               1
#define PROHIBIDO_PULSO_MS             2000UL

static_assert(INTERVALO_GIRO_MS >= SEPARACION_MINIMA_EVENTOS_MS,
              "El pulso de orientacion no puede repetirse antes de la separacion minima");

// ── Cambio de luz ────────────────────────────────────────────────────────────
// Un cambio de luz sólo cuenta cuando llegan estos paquetes SEGUIDOS con la luz
// nueva. Un paquete suelto que contradice a los demás (alterado por
// interferencia u otro equipo) no hace vibrar. Con 2, el aviso se retrasa
// medio segundo (un envío del semáforo).
#define CONFIRMACIONES_CAMBIO_LUZ      2

// ── Orientación frente al cruce ──────────────────────────────────────────────
// La persona queda "de frente" cuando |Δθ − 180°| < UMBRAL_CRUCE_GRADOS (config.h)
// y sólo vuelve a "mal orientada" cuando supera UMBRAL + HISTERESIS. Sin esta
// banda, una lectura que oscila alrededor del umbral alternaría los dos estados y
// cada vuelta a "de frente" repetiría el patrón del semáforo sin un evento real.
// Con el umbral en 5°, se sale de "de frente" al pasar de 10°.
#define HISTERESIS_ORIENTACION_GRADOS  5.0f

// La orientación va antes que la luz: para quedar "de frente" hacen falta
// estas lecturas SEGUIDAS bajo el umbral (2 = 0.4 s a 5 Hz). Mientras se
// confirma no se pide ningún pulso de giro. Mientras la luz no se avisó, el
// cono es el de UMBRAL_CRUCE_GRADOS: la banda de histéresis sólo protege un
// aviso ya dado, así nunca suena la luz con la persona ya pasada del cono.
#define CONFIRMACIONES_ORIENTACION     2

// Si la persona ya está girando hacia el cruce no se repite el pulso de giro:
// el pulso cada 3 s le decía "sigue girando" justo cuando estaba por llegar,
// y con los motores ocupados el aviso de la luz (la señal de "para") llegaba
// hasta 3 s tarde, ya pasada del otro lado. "Acercándose" = la desviación bajó
// al menos UMBRAL_ACERCAMIENTO_GRADOS a lo largo de LECTURAS_ACERCAMIENTO
// lecturas seguidas, todas del mismo lado (1° en 0.4 s ≈ 2.5°/s).
#define LECTURAS_ACERCAMIENTO          3
#define UMBRAL_ACERCAMIENTO_GRADOS     1.0f

static_assert(CONFIRMACIONES_ORIENTACION >= 1, "Hace falta al menos una lectura para quedar de frente");

#endif // CONSTANTES_H
