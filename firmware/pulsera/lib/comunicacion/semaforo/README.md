# 📡 lib/comunicacion/semaforo
El módulo **lib/comunicacion/semaforo** contiene la lógica de la comunicación
de la pulsera con el semáforo mediante ESP-NOW.

--- 

## 📂 Contenido
El módulo está compuesto por los archivos:
- **semaforo.h**: Define la estructura de la clase *ComunicadorSemaforo*.
- **semaforo.cpp**: Se encarga del desarrollo de la clase *ComunicadorSemaforo*.

---

## ⚙️ Explicación
- **Clase *ComunicadorSemaforo***: Esta clase se encarga de recibir las lecturas
  que provienen del semáforo y convertirlas en la estructura *ultimoEstado*
- **Atributos**: La clase ***ComunicadorSemaforo*** contiene los atributos
  *BROADCAST_MAC*, un atributo privado que contiene la dirección MAC del dispositivo con el que se
  está comunicando (Dirección MAC del broadcast), también tiene como atributo
  la estructura *ultimoEstado*, atributo público responsable de almacenar los datos que el semáforo
  está enviando.
- **Métodos**: La clase ***ComunicadorSemaforo*** contiene los siguientes
  métodos:
  - **inicializar( )**: Se encarga de inicializar el dispositivo y
    prepararlo para la comunicación mediante ESP-NOW
  - **desinicializar( )**: Se encarga de apagar el dispositivo y su
    comunicación por ESP-NOW
  - **onDatosRecibidos( )**: Callback para convertir las lecturas
    del semáforo en la estructura *estado_semaforo* que será guardada
    posteriormente en *ultimo_estado*.

---

## 🔗 Dependencias
El módulo **lib/comunicacion/semaforo** tiene las siguientes dependencias:
- **<esp_now.h>**: Es el módulo encargado de la comunicación mediante
  el protocolo ESP-NOW.
- **<WiFi.h>**: Es el módulo que permite usar el chip WiFi, es fundamental
  para poder utilizar ESP-NOW.

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

- **ultimoEstado (Lectura de datos)**:
  ```
    //Leemos grados de ultimoEstado
    ComunicadorSemaforo::ultimoEstado.grados;
    //Leemos permitidoCruce de ultimoEstado
    ComunicadorSemaforo::ultimoEstado.permitidoCruce
  ```
- *Nota*: El metodo *onDatosRecibidos*, al ser una callback es pasado como argumento
para la funcion *esp_now_register_recv_cb* durante la inicializacion.
