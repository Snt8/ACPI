# lib/sensores
El modulo **lib/sensores** contiene la logica para inicializar, leer y corregir la informacion
de los sensores que utiliza la pulsera. Los sensores utilizados son el **MPU6050** (Acelerometro) y el
**QMC5883l** (Magnetometro).

--- 

## Contenido
El módulo está compuesto por los siguientes archivos:
- **mpu6050.h**: Define la estructura de la clase ***ControladorAcelerometro***.
- **mpu6050.cpp**: Se encarga del desarrollo de la clase ***ControladorAcelerometro***.
- **qmc5883l.h**: Define la estructura de la clase ***ControladorMagnetometro***.
- **qmc5883l.h**: Se encarga del desarrollo de la clase ***ControladorMagnetometro***.

---

## Explicacion
- ### Acelerometro:
    - **Clase *ControladorAcelerometro***: Esta clase se encarga de la inicializacion y lectura de los datos que 
    provienen del acelerometro.
    - **Atributos de la clase**: Esta clase está compuesta por los atributos privados que representan
    los ejes de la *brujula (x, y, z)*, representados mediante decimales.
    - **Metodos de la clase**: La clase ***ControladorAcelerometro*** contiene los siguientes metodos:
      - **inicializar( )**: Se encarga inicializar el sensor mediante el bus I2C.
      - **leerDatos()**: Se encarga de efectuar la lectura de los datos del sensor y convertirlos
      de bits a decimales para asignarlos a los atributos de los *ejes de la brujula*.
      - **obtenerPitchRoll( )**: Se encarga de calcular *la inclinacion hacia adelante, atras y 
      lateral* del sensor, permitiendo conocer la inclinacion de la pulsera en la muñeca. Este metodo
      recibe como argumento por referencia las variables *pitch* y *roll* para modificarlas dentro del metodo
      y asignarles los datos de la inclinacion.
- ### Magnetometro:
    - **Clase *ControladorMagnetometro***: Esta clase se encarga de la inicializacion y lectura de los datos que 
    provienen del magnetometro.
    - **Atributos de la clase**: Esta clase está compuesta, al igual que el *acelerometro*, por los atributos 
    privados que representan los *ejes de la brujula* (x, y, z).
    - **Metodos de la clase**: La clase ***ControladorMagnetometro** contiene los siguientes metodos:
        - **inicializar( )**: Se encarga de inicializar el sensor mediante el bus I2C.
        - **leerPosicion( )**: Se encarga de leer la posicion del *magnetometro*, hacer las operaciones 
        de bits para convertirlos a decimales y asignarlos a los atributos de los *ejes de la brujula*.
        - **calcularHeading( )**: Se encarga de corregir mediante trigonometria los *ejes de la brujula*, 
        permitiendo que la brujula no se vea afectada por la inclinacion natural que tendria la pulsera en 
        la muñeca. Recibe como argumento los valores decimales *pitch* y *roll*, que se obtuvieron con el
        *acelerometro*.

---

## Dependencias
El módulo **lib/sensores** tiene las siguientes dependencias:
- **<Wire.h>**: Es el módulo que permite inicializar el bus I2C con el que establecemos la comunicacion con el 
sensor.

---

## Ejemplo de Uso
- ### ControladorAcelerometro: 
    - **inicializar( )**:
    ```
    if(!ControladorAcelerometro::inicializar()){
        //Logica en caso de que falle la inicializacion
        //Aprovechamos el retorno booleano para verificar la inicializacion
    }
    ```
    - **leerDatos( )**:
    ```
    //Asigna las lecturas en los atributos estaticos de la clase
    ControladorAcelerometro::leerDatos();
    ```
    - **obtenerPitchRoll( )**:
    ```
    //Declaramos las variables que pasaremos por referencia
    float pitch = 0.0;
    float roll = 0.0;
    //Llamamos al metodo que modificara las variables en base a las lecturas del acelerometro
    ControladorAcelerometro::obtenerPitchRoll(pitch, roll);
    ```
  - ### ControladorMagnetometro:
      - **inicializar( )**:
      ```
      if(!ControladorMagnetometro::inicializar()){
          //Logica en caso de que falle la inicializacion
          //Igual que con el acelerometro, aprovechamos el retorno booleano
      }
      ```
      - **leerDatos( )**:
      ```
      //Se asignan los valores de las lecturas en los atributos del clase
      ControladorMagnetometro::leerDatos();
      ```
      - **calcularHeading( )**:
      ```
        //Retornara el angulo corregido al que esta orientado el usuario
        float angulo = ControladorMagnetometro::calcularHeading(pitch, roll);
        Serial.println(angulo);
        >> 40.0 
      ``` 