# 🧭 lib/brujula
El módulo **lib/brujula** contiene la lógica de dirección que
permite a la pulsera determinar si el usuario está correctamente
ubicado para permitirle cruzar la calle.

---

## 📂 Contenido
El módulo está compuesto por los archivos:
- **brujula.h**: Define la estructura de la clase
  ***ControladorBrujula***.
- **brujula.cpp**: Se encarga del desarrollo de la clase ***ControladorBrujula***.

---

## ⚙️ Explicación
- **Clase *ControladorBrujula***: Esta clase es un orquestador
  de los sensores y su responsabilidad principal es encargarse
  de la dirección y permitir a la pulsera determinar si el cruce
  puede efectuarse con base en dónde esté orientado el usuario.
  Esta clase es estática, por ende no requiere ser instanciada.
- **Atributos de la clase**: La clase ***ControladorBrujula*** contiene el
  atributo *headingActual*, este atributo público es un decimal que representa
  la orientación del usuario en grados.
- **Métodos de la clase**: La clase ***ControladorBrujula*** contiene
  los siguientes métodos estáticos:
  - **inicializar( )**: Se encarga de llamar a los métodos de inicialización
    de los sensores.
  - **obtenerHeading( )**: Se encarga de comunicarse con los sensores,
    obtener las lecturas y calcular el heading compensado para ser asignado
    en el atributo *headingActual*.
  - **revisarSemaforo( )**: Se encarga de comparar *headingActual* de la pulsera
    con el heading del semáforo para determinar que ambos sean similares,
    verificando que el usuario está bien orientado para el cruce. Recibe
    la posición del semáforo como argumento.
  
---

## 🔗 Dependencias
El módulo **lib/brujula** tiene las siguientes dependencias:
- **../sensores/mpu6050.h**: Este archivo contiene la declaración de
  la clase ControladorAcelerometro, clase encargada de las lecturas
  del acelerómetro.
- **../sensores/qmc5883l.h**: Este archivo contiene las declaraciones de
  la clase ControladorMagnetómetro, clase encargada de las lecturas
  del magnetómetro.
- **../utils/calcular_direccion.h**: Este archivo contiene la declaración de la
  función calcularDireccion, que se encarga de calcular la diferencia entre
  el heading de la pulsera y el del semáforo.

---

## 💻 Ejemplo de Uso
Para los ejemplos, analizaremos cómo usar cada uno de los métodos de la clase ***ControladorBrujula***:
- **inicializar( )**:
  ```
  if(!ControladorBrujula::inicializar()){
    //Logica en caso de que falle la inicializacion
    //Aprovechamos el retorno booleano para evaluar que todo sea correcto
  }
  ```
- **obtenerHeading( )**:
  ```
  ControladorBrujula::obtenerHeading();
  //Modifica el atributo headingActual que posteriormente podemos leer
  Serial.println(ControladorBrujula::headingActual);
  // >> 45.0
  ```
  - **revisarSemaforo( )**:
  ```
  ControladorBrujula::revisarSemaforo(posicionSemaforo);
  //Retorna true si la comparacion de los headings permite el cruce
  ```