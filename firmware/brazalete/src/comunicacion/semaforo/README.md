# 📡 src/comunicacion/semaforo
El módulo **src/comunicacion/semaforo** contiene la lógica de la comunicación
del brazalete con el semáforo mediante ESP-NOW.

---

## 📂 Contenido
El módulo está compuesto por los archivos:
- **semaforo.h**: Define la estructura de la clase *ComunicadorSemaforo*.
- **semaforo.cpp**: Se encarga del desarrollo de la clase *ComunicadorSemaforo*.

---

## ⚙️ Explicación
- **Clase *ComunicadorSemaforo***: Esta clase se encarga de recibir los paquetes
  que provienen del semáforo y dejarlos listos para que la FSM los lea desde `loop()`.
- **Recepción segura**: el callback de ESP-NOW corre en la tarea de WiFi, no en
  `loop()`. Sólo acepta paquetes con el tamaño exacto de `estado_semaforo` y la firma
  `"ACPI"` (`esPaqueteSemaforo()`, en `src/protocolo/protocolo_esp.h`); los demás se
  cuentan como descartados. El paquete válido queda en un buffer protegido con una
  sección crítica, así toda la lógica corre en un único hilo. Sólo se guarda el último
  paquete: si llegan dos antes de que la FSM lea, el anterior ya no sirve.
- **Encendido y apagado de la radio**: el modo standby enciende y apaga la radio cada
  5 s (ver `src/control/modo_standby.h`), así que `inicializar()` y `desinicializar()`
  se pueden llamar dos veces seguidas sin efecto.
- **Métodos**: La clase ***ComunicadorSemaforo*** contiene los siguientes
  métodos:
  - **inicializar( )**: pone el WiFi en modo estación y empieza a escuchar ESP-NOW.
  - **desinicializar( )**: deja de escuchar y apaga el WiFi por completo.
  - **tomarPaquete( )**: copia el último paquete válido recibido y lo marca como leído.
    Devuelve `false` si no llegó ninguno desde la última llamada.
  - **hayPaquete( )**: indica si hay un paquete sin tomar (lo usa el standby).
  - **descartarPendiente( )**: olvida el paquete sin tomar, si lo hay.
  - **paquetesDescartados( )**: cuántos paquetes ESP-NOW se rechazaron por tamaño o
    firma (sale en el heartbeat del monitor serie).
  - **onDatosRecibidos( )**: callback privado que valida el paquete y lo deja en el
    buffer.

---

## 🔗 Dependencias
El módulo **src/comunicacion/semaforo** tiene las siguientes dependencias:
- **<esp_now.h>**: Es el módulo encargado de la comunicación mediante
  el protocolo ESP-NOW.
- **<WiFi.h>**: Es el módulo que permite usar el chip WiFi, es fundamental
  para poder utilizar ESP-NOW.
- **src/protocolo/protocolo_esp.h**: estructura `estado_semaforo` y validación de la firma.

---

## 💻 Ejemplo de Uso
Para los ejemplos, analizaremos cómo usar cada uno de los métodos de la clase ***ComunicadorSemaforo***:
- **inicializar( )**:
  ```
  if(!ComunicadorSemaforo::inicializar()){
    //Logica en caso que falle la inicializacion
    //Aprovechamos el retorno booleano para evaluar que todo sea correcto
  }
  ```

- **desinicializar( )**:
  ```
  if(!ComunicadorSemaforo::desinicializar()){
    //Logica en caso que falle la desinicializacion
    //Tambien posee un retorno booleano, por lo que lo usamos igual
  }
  ```

- **tomarPaquete( ) (Lectura de datos, desde loop())**:
  ```
  estado_semaforo paquete;
  if (ComunicadorSemaforo::tomarPaquete(paquete)) {
    paquete.grados;          //Heading del semáforo
    paquete.permitidoCruce;  //1 = el peatón puede cruzar
  }
  ```
- *Nota*: El metodo *onDatosRecibidos*, al ser una callback es pasado como argumento
para la funcion *esp_now_register_recv_cb* durante la inicializacion.
