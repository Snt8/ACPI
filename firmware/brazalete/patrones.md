# Patrones de vibración del brazalete

Especificación aprobada el 27 oct 2026. La implementan:

| Pieza | Archivo | Qué hace |
|---|---|---|
| `ControladorPatrones` | `src/vibracion/controlador_patrones.*` | Reproduce **una vez** el patrón pedido y hace cumplir la separación mínima de 3 s. |
| `EvaluadorSemaforo` | `src/control/evaluador_semaforo.*` | Compara cada paquete del semáforo con el anterior, sigue la orientación de la persona y decide qué patrón pedir. |
| `MaquinaEstados` | `src/control/fsm.cpp` | Pasa paquetes y lecturas al evaluador y aplica a los motores lo que dice el controlador. |

Los tiempos están en `src/constantes.h`. Las pruebas que verifican este documento
están en `test/` (ver `test/README.md`).

"Pulso" significa motor encendido a su intensidad (`INTENSIDAD_MOTOR` = 120/255 para
el derecho, `INTENSIDAD_MOTOR_IZQUIERDO` = 135/255 para el izquierdo); entre pulsos el
motor queda apagado.

Leyenda de los diagramas: `█` = vibrando, `·` = apagado, cada carácter = 100 ms.

---

## Reglas que protegen las soldaduras

Las soldaduras de los motores son débiles, así que estas reglas no tienen excepciones:

1. **Sin evento no hay vibración.** Los motores sólo se encienden mientras suena un
   patrón, y un patrón sólo empieza por uno de los eventos de abajo.
2. **Cada evento vibra una sola vez.** Ningún patrón se repite solo. El único que se
   repite es el de orientación, y lo hace porque cada 3 s con la persona mal orientada
   cuenta como un evento nuevo.
3. **Al menos 3 s entre eventos.** Entre el inicio de un patrón y el del siguiente
   pasan como mínimo `SEPARACION_MINIMA_EVENTOS_MS` (3 s), y dos patrones nunca se
   solapan. Si llega un evento antes, espera su turno.
   Esto incluye al rojo: en el borrador se proponía que cortara a cualquier otro
   patrón, pero eso rompería esta regla. El semáforo cambia de luz cada 25 s, así que
   la espera máxima de un aviso es de unos pocos segundos.

---

## 1. Semáforo detectado: 3 pulsos intermitentes, ambos motores

```
Ambos  ██·····██·····██
       0      0.7    1.4 1.6 s
```

- 3 pulsos de **200 ms** con **0.5 s de pausa** entre uno y otro (1.6 s en total).
- **Evento:** primer paquete de un semáforo desde que el brazalete salió de standby.
  El brazalete se queda con ese semáforo (su `id`) hasta volver a standby; los paquetes
  de otro `id` se ignoran, para que dos semáforos cercanos no alternen y repitan este
  aviso.

## 2. Orientación: 1 pulso del motor hacia donde hay que girar

```
Derecho   ███···························███···   (girar a la derecha)
Izquierdo ··································
          0                             3.0 s
```

- 1 pulso de **300 ms** sólo en el motor del lado hacia el que hay que girar.
- **Evento:** la persona no está de frente al cruce. Se repite **cada 3 s** mientras
  siga mal orientada y se detiene en cuanto queda de frente.
- "Derecho" e "izquierdo" son los motores de `PIN_MOTOR_DERECHO` (GPIO10) y
  `PIN_MOTOR_IZQUIERDO` (GPIO3). Si al probar resultan invertidos respecto a la
  persona, se cambian los pines en `config.h`.

## 3. Semáforo en verde (cruce permitido): 2 pulsos sostenidos, ambos motores

```
Ambos  ████████████████████·····████████████████████
       0                  2.0  2.5                4.5 s
```

- 2 pulsos de **2 s** con **0.5 s de pausa** (4.5 s en total).
- **Evento:** la persona queda de frente al cruce con `permitidoCruce = 1`, o la luz
  cambia a verde peatonal mientras está de frente.

## 4. Semáforo en rojo (cruce prohibido): 1 pulso sostenido, ambos motores

```
Ambos  ████████████████████
       0                  2.0 s
```

- 1 pulso de **2 s**.
- **Evento:** la persona queda de frente con `permitidoCruce = 0`, o la luz cambia a
  rojo peatonal mientras está de frente.

> "Verde" y "rojo" se refieren al **semáforo peatonal**. En el paquete,
> `permitidoCruce = 1` significa rojo para los autos, es decir, verde para la persona.

---

## Orden de los eventos

```
STANDBY ──primer paquete "ACPI"──▶ [1] detectado
                                      │
                     ¿de frente al cruce?
                       │ no                    │ sí
                       ▼                       ▼
         [2] pulso cada 3 s          ¿ya se avisó esta luz?
           hasta quedar de frente      no ─▶ [3] verde / [4] rojo
                                       sí ─▶ silencio
```

- **De frente** significa `|Δθ − 180°| < 30°` (`UMBRAL_CRUCE_GRADOS`). Para volver a
  "mal orientada" hay que pasar de 40° (`+ HISTERESIS_ORIENTACION_GRADOS`). Sin esta
  banda, una lectura que oscila alrededor de 30° haría repetir el aviso de la luz sin
  un evento real.
- **Al volver a quedar de frente** después de haberse desviado (más de 40°), se avisa
  otra vez la luz actual. Es un evento real: la persona se había salido del cruce.
- **Cambio de luz con la persona mal orientada:** no vibra en ese momento; la luz
  nueva se avisa cuando queda de frente.
- **Paquetes sin cambios** (misma luz; sólo cambian los grados o el tiempo restante)
  no son eventos.

## Brújula y motores

- **Lectura inválida** (giro brusco de más de 40° en 0.5 s): no empieza ningún pulso de
  orientación, y si estaba sonando uno se corta, porque podría indicar el lado
  equivocado. Cortarlo no adelanta el siguiente patrón: los 3 s se cuentan desde su
  inicio. Los avisos de luz no se cortan.
- **Mientras vibra un motor no se lee la brújula:** los imanes de los motores alteran
  el magnetómetro. Se usa la última lectura buena.

## Standby

Tras `TIEMPO_SIN_PAQUETES_STANDBY_MS` (30 s) sin paquetes del semáforo, el brazalete
corta cualquier patrón, apaga los motores y olvida el semáforo. Al encontrarlo de nuevo
vuelve a sonar el aviso 1, respetando igualmente los 3 s desde el último patrón.
