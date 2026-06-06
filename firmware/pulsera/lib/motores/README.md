# 📳 lib/motores
El módulo **lib/motores** contiene la lógica de vibración de los motores,
permitiendo a la pulsera dar feedback háptico al usuario sobre su orientación
y sobre el estado del semáforo.

--- 

## 📂 Contenido
El módulo está compuesto por los siguientes archivos:
- **motor.h**: Define la estructura de la clase ***ControladorMotores***.
- **motor.cpp**: Se encarga del desarrollo de la clase ***ControladorMotores***.

--- 

## ⚙️ Explicación
- **Clase *ControladorMotores***: Esta clase se encarga de la inicialización de los motores y la gestión de la vibración.
- **Atributos de la clase**: La clase ***ControladorMotores*** está compuesta por los atributos
  privados de configuración de los motores, entre ellos los *pines* y los *canales* de los motores,
  la *frecuencia*, encargada de la velocidad de oscilación, y la *resolución*. Como atributo público, la
  clase ***ControladorMotores*** define una colección **Motor** que contiene las constantes de enumeración de cada motor.
- **Métodos de la clase**: La clase ***ControladorMotores*** contiene los siguientes métodos:
  - **inicializar( )**: Se encarga de conectar los pines e inicializar los motores.
  - **vibrarMotor( )**: Se encarga de encender los motores, recibe como argumento la constante de enumeración del motor
    y la intensidad a la que queremos que vibre el motor.
  - **detenerVibracion( )**: Se encarga de detener la vibración de los motores, recibe como argumento la constante de enumeración
    del motor que queremos detener.

---

## 🔗 Dependencias
El módulo **lib/motores** tiene las siguientes dependencias:
- **<Arduino.h>**: Es el módulo que contiene las funciones que nos permiten tanto enlazar los motores a los pines,
  activarlos y desactivarlos.

--- 

## 💻 Ejemplo de Uso
Para los ejemplos de uso, analizaremos cómo usar cada uno de los métodos de la clase ***ControladorMotores***:
- **inicializar( )**:
  ```
    if(!ControladorMotores::inicializar()){
        //Logica en caso de que falle la inicializacion de algun motor
        //El metodo tiene un retorno booleano, que usamos para confirmar la inicializacion
    }
  ```
- **vibrarMotor( )**:
  ```
    //Hacemos vibrar el motor derecho con 255 de intensidad
    ControladorMotores::vibrarMotor(ControladorMotores::DERECHO, 255);
    //Hacemos vibrar el motor izquierdo con 255 de intensidad
    ControladorMotores::vibrarMotor(ControladorMotores::IZQUIERDO, 255);
  ```
  - **detenerMotor( )**:
  ```
    //Detenemos ambos motores
    ControladorMotores::detenerMotor(ControladorMotores::AMBOS);
  ```
