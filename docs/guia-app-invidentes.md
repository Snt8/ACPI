# Guía de diseño: app ACPI para personas ciegas (React Native)

Documento de diseño previo a la reescritura de la app móvil (hoy Android nativa en
Kotlin, carpeta `app/`) en **React Native para iOS y Android**. Recoge la
investigación hecha el 28 oct 2026 y las decisiones que salen de ella. Es la
referencia para implementar; si algo cambia en el camino, se actualiza aquí.

---

## 1. Conclusión de la investigación (resumen)

**La app no se debe diseñar alrededor de comandos de voz.** Se debe diseñar para
funcionar de forma impecable con los lectores de pantalla del sistema
(**VoiceOver** en iOS, **TalkBack** en Android) y, sobre todo, para que **casi no
haga falta tocarla**: el brazalete ya guía con vibración, así que el teléfono va en el
bolsillo y la app trabaja sola en segundo plano.

Los motivos:

1. **Las personas ciegas ya usan el teléfono con VoiceOver o TalkBack.** Un estudio
   de uso real con usuarios expertos de TalkBack (CHI 2021) muestra que navegan con
   gestos de deslizar y usan la voz sobre todo para *escribir* texto, no para manejar
   apps. Una interfaz propia "por voz" que ignore el lector de pantalla obliga a
   aprender algo nuevo y suele chocar con él (los dos hablan a la vez).
2. **La voz es mala en la calle.** En un cruce hay ruido de tráfico (el reconocimiento
   falla), la persona necesita **los oídos libres para escuchar los autos**, y hablarle
   al teléfono en público expone a la persona. Tampoco sirve como única vía: en Android
   el reconocimiento depende del fabricante y del idioma instalado.
3. **Durante el cruce no hay manos libres**: una mano lleva el bastón o el perro guía.
   Por eso la información crítica la da el brazalete (háptica) y la app sólo complementa
   con avisos breves y opcionales.
4. **Los gestos propios chocan con el lector de pantalla.** TalkBack intercepta todos
   los gestos y VoiceOver se queda con el doble toque; las guías del W3C recomiendan no
   inventar gestos y dar siempre un botón equivalente.

Por eso la voz entra **como atajo opcional** (fase 3), no como interfaz principal:
- activar con un **toque mágico** (doble toque con dos dedos en iOS) o un botón grande,
  nunca con una palabra de activación siempre escuchando;
- pocos comandos cortos y fijos ("estado", "repetir", "batería", "silencio", "ayuda");
- y, más adelante, atajos del sistema: **Siri** (paquete `expo-app-intents`, aún en
  alfa) para "Oye Siri, estado de ACPI".

Apps de referencia que siguen esta línea: **OKO** (detecta el semáforo con la cámara y
avisa con sonido + vibración, desarrollada junto a usuarios ciegos) y **Microsoft
Soundscape** (audio breve, pensado para auriculares de conducción ósea y para no tapar
el sonido de la calle; su código quedó abierto al cerrarse el proyecto).

---

## 2. Qué hace la app (y qué no)

El brazalete decide y vibra por su cuenta (ver `firmware/brazalete/patrones.md`). La app
es un **acompañante**:

| Hace | No hace |
|---|---|
| Se conecta sola al brazalete cuando está cerca de un semáforo, también con la pantalla apagada. | No da la orden de cruzar. Informa del estado; la persona decide. |
| Avisa por voz (opcional) los cambios importantes: semáforo detectado, paso permitido, paso prohibido, conexión perdida, batería baja. | No repite en voz lo que el brazalete ya dice con vibración en cada paquete (cada 500 ms). |
| Responde "¿cuál es el estado?" en una sola frase. | No pide mirar la pantalla para nada importante. |
| Guarda un historial local de cruces y (si se configura) envía métricas a Grafana. | No escucha el micrófono de forma continua. |
| Tiene una pantalla de diagnóstico para el equipo (pruebas de motor, datos crudos). | No reemplaza el bastón, el perro guía ni el oído. |

**Regla de redacción:** los mensajes describen el estado, nunca ordenan. "Paso
peatonal permitido, quedan 12 segundos", no "Cruce ahora". Lo mismo en la UI, la
documentación y el informe.

---

## 3. Protocolo BLE que se conserva

Sin cambios respecto a `docs/ble-protocol.md` y
`firmware/brazalete/src/comunicacion/movil/movil.cpp`:

- Nombre anunciado: `ACPI_Brazalete`.
- Servicio `12345678-1234-1234-1234-123456789abc` (se anuncia en el *advertising*, lo
  que permite el escaneo en segundo plano en iOS, que exige filtrar por UUID).
- TX (notify) `87654321-4321-4321-4321-cba987654321`: JSON cada 500 ms. Desde el 29
  oct 2026 es la versión 2, que agrega a `{"hd":135,"st":1,"ok":true,"tr":18,"bt":-1}`
  la secuencia, el semáforo, la luz, el giro, el estado de la brújula (`br = 0`: sin
  calibrar, la app lo avisa por voz), el patrón en curso y la recepción y señal del
  semáforo. Los campos de la versión 1 no cambiaron. Detalle en `docs/ble-protocol.md`.
- RX (write) `11111111-2222-3333-4444-555555555555`: `TEST_ALERT` (vibración de
  prueba, respeta los 3 s entre patrones) y `VIBRATE_STOP` (corta el patrón).

Lo que hace hoy el firmware (`fsm.cpp`, `enviarTelemetria`) y debe respetar la app:

| `st` | Significado real | Mensaje de voz (primera vez que aparece) |
|---|---|---|
| 0 | De frente al cruce, paso peatonal **prohibido** | "Paso peatonal en rojo, quedan N segundos" |
| 1 | De frente al cruce, paso peatonal **permitido** | "Paso peatonal en verde, quedan N segundos" |
| 2 | Mal orientado (el brazalete ya vibra hacia dónde girar) | Nada por voz por defecto; en modo detallado: "Orientación incorrecta" |
| 3 | Pánico (**documentado pero el firmware no lo envía**; no hay botón) | Reservado. La app debe tolerar este y cualquier otro valor desconocido. |

- `bt` hoy siempre es `-1` (sin medición de batería): la app lo muestra como "no
  disponible", sin alertas.
- **El brazalete apaga el BLE en standby** (30 s sin semáforo). La conexión sólo existe
  cerca de un semáforo, así que la app debe **re-escanear y reconectar sola**, y una
  desconexión al alejarse es normal: no se anuncia como error salvo que ocurra con un
  semáforo a la vista (estado `st` reciente) o en mitad de un cruce.
- El parser debe ser tolerante: claves faltantes, JSON partido o valores fuera de rango
  se ignoran sin cerrar la conexión.

---

## 4. Principios de diseño accesible

### 4.1 Lector de pantalla primero
- Cada control tiene `accessibilityLabel` (qué es), `accessibilityHint` cuando la
  acción no es obvia, y `accessibilityRole` correcto (`button`, `header`, `switch`,
  `adjustable`…). Nada de botones que sólo sean un icono sin etiqueta.
- **Agrupar** la información que va junta en un solo elemento enfocable
  (`accessible={true}` en el contenedor): la tarjeta de estado se lee como una frase,
  "Conectado. Paso peatonal en verde, quedan 12 segundos", no como cinco elementos.
- El orden del foco sigue el orden de importancia: estado → acción principal → resto.
- Los cambios de estado se anuncian: `AccessibilityInfo.announceForAccessibility` en
  iOS y `accessibilityLiveRegion="polite"` (o `assertive` para conexión perdida) en
  Android.
- Acciones extra sin gestos propios: `accessibilityActions` (aparecen en el rotor de
  VoiceOver y en el menú de acciones de TalkBack) y `onMagicTap` en iOS para "repetir
  estado".
- **No inventar gestos.** Todo lo que se pueda hacer con un gesto también se puede
  hacer con un botón.

### 4.2 Un solo canal de voz a la vez
Si el lector de pantalla está activo y la app está en primer plano, los avisos van por
el lector (`announceForAccessibility`) para no hablar encima de él. Si la app está en
segundo plano o no hay lector, van por síntesis de voz (`expo-speech`). Se detecta con
`AccessibilityInfo.isScreenReaderEnabled()` y su evento de cambio.

Reglas del audio:
- Frases de **menos de 2 segundos**, la información importante primero.
- Prioridades: conexión perdida durante un cruce > cambio de luz > semáforo detectado >
  batería. Un mensaje nuevo de mayor prioridad corta al anterior; uno de menor
  prioridad espera o se descarta si ya no es cierto.
- Sin repetir: cada evento se dice una vez (igual que el brazalete, que vibra una vez
  por evento).
- *Ducking*: bajar la música/podcast mientras se habla, no pausarla.
- Recomendar auriculares de conducción ósea o uno solo, para no tapar el tráfico.
- Velocidad de voz configurable (muchos usuarios expertos escuchan muy rápido).

### 4.3 Baja visión (no todos los usuarios son ciegos totales)
- Tema oscuro de alto contraste (mínimo 7:1 en el texto de estado), texto que respeta
  el tamaño de fuente del sistema (Dynamic Type / escala de fuente) sin cortarse.
- El estado nunca se comunica sólo con color: siempre texto + icono + forma.
- Objetivos táctiles de al menos 48×48 dp (Android) / 44×44 pt (iOS); el botón
  principal ocupa buena parte de la pantalla.

### 4.4 Menos pasos
- Sin cuentas, sin login, sin formularios.
- Emparejamiento automático por nombre `ACPI_Brazalete`; la lista de dispositivos sólo
  aparece si hay más de uno.
- Los permisos se piden con una explicación hablada antes del diálogo del sistema y,
  si se niegan, la app dice exactamente qué ajuste abrir.

---

## 5. Pantallas

Navegación por pestañas inferiores con texto (máximo 3): **Estado**, **Semáforos** y
**Ajustes**. Diagnóstico e Historial cuelgan de Ajustes.

### 5.1 Estado (pantalla principal)
- **Tarjeta de estado** (un solo elemento accesible, región viva): conexión +
  estado del cruce + segundos restantes.
- **Botón grande "Repetir estado"** (también toque mágico en iOS): dice la frase de
  estado actual.
- Botón "Silenciar avisos de voz" (switch, anuncia su estado).
- Fase 3: botón "Hablar" para comandos de voz.

### 5.2 Semáforos (agregado el 28 oct 2026)
La persona (o quien la acompañe) guarda en el teléfono los semáforos que usa.
- **Botón "Guardar semáforo aquí"**: un toque, parado junto al semáforo. Toma la
  posición del GPS, sugiere un nombre con la dirección ("Semáforo en Av. Juárez 120") y
  lo anuncia. Si la precisión del GPS es mala (> 20 m) lo dice.
- **Lista** ordenada por distancia, cada fila como una frase: "Semáforo en Av. Juárez
  120, a 80 metros al noreste". Tocar una fila abre la edición (nombre, notas, borrar).
- **Mapa** (OpenStreetMap) para quien ve: marcadores (tocar uno abre su edición), toque
  largo para marcar un semáforo en otro punto y arrastrar el marcador para corregirlo. **Se oculta al lector de
  pantalla**; la lista es su equivalente accesible.
- Todo se guarda localmente (SQLite), sin cuentas ni servidor.
- **Semáforos detectados**: cuando el brazalete encuentra un semáforo, la app toma su
  ubicación con el GPS mientras la persona espera. Si no hay uno guardado a menos de 25 m,
  lo agrega a la lista ("Detectado por el brazalete"). Ver `docs/grafana-setup.md`.
- Más adelante: avisar al acercarse a un semáforo guardado y, si el firmware agrega el
  `id` del semáforo a la telemetría BLE, unir cada paquete con su semáforo guardado.

### 5.3 Historial (dentro de Ajustes)
- Lista de cruces: fecha, semáforo, tiempo de espera, si hubo desconexión. Cada fila
  es un solo elemento con una frase completa.

### 5.4 Ajustes
- Nivel de avisos: *Silencioso* (sólo conexión perdida), *Básico* (por defecto),
  *Detallado* (incluye orientación).
- Velocidad y voz de la síntesis; vibración del teléfono además del brazalete (por
  defecto apagada para no confundir).
- Tutorial (volver a escucharlo).
- Acceso a Diagnóstico: entrada con etiqueta clara "Diagnóstico (para técnicos)".

### 5.5 Diagnóstico (equipo ACPI)
Equivalente al `BLEDebugFragment` actual: datos crudos, rumbo, botones `TEST_ALERT` y
`VIBRATE_STOP`, log de conexión. También accesible, pero no forma parte del flujo
normal.

### 5.6 Primer uso (tutorial)
3–4 pasos hablados: qué es ACPI, permisos de Bluetooth y notificaciones, cómo suena
cada aviso (con botón "probar"), y prueba de vibración del brazalete si está cerca.

---

## 6. Funcionamiento en segundo plano

Es la parte técnicamente más delicada, porque la app tiene que reconectar sola cuando
el brazalete sale de standby.

- **iOS:** modo de fondo `bluetooth-central` + restauración de estado de CoreBluetooth
  (`restoreStateIdentifier`), escaneo filtrado por el UUID del servicio. Para hablar en
  segundo plano también hace falta el modo `audio` y una sesión de audio configurada
  para mezclar con otras apps (*duck others*).
- **Android:** servicio en primer plano de tipo `connectedDevice` con notificación
  persistente ("ACPI activo"), permisos `BLUETOOTH_SCAN` (con
  `neverForLocation`) y `BLUETOOTH_CONNECT` en Android 12+, y pedir excluir la app de
  la optimización de batería (algunos fabricantes matan servicios igual; documentarlo en
  el tutorial).
- Reintentos de conexión con espera creciente (2 s, 5 s, 10 s… máximo 30 s) mientras
  la app esté activa.

---

## 7. Tecnología

| Necesidad | Elección | Nota |
|---|---|---|
| Base | **Expo** (SDK estable más reciente) + TypeScript, *development build* (no Expo Go, porque BLE es nativo) | `npx create-expo-app@latest` |
| Navegación | `expo-router` (pestañas) | |
| BLE | `react-native-ble-manager` (v12) | Elegida al instalar (SDK 58): sólo soporta la New Architecture, tiene plugin de Expo y se publica seguido; `react-native-ble-plx` no publicaba desde feb 2026. Aislada detrás de `ClienteBrazalete` para poder cambiarla. |
| Mapa | `@maplibre/maplibre-react-native` + estilo "liberty" de [OpenFreeMap](https://openfreemap.org) | Open source, el mismo mapa en iOS y Android, datos de OpenStreetMap y sin clave ni cuenta. Se descartó Google Maps (pide cuenta de facturación en Google Cloud) y `expo-maps` (en iOS sólo usa Apple Maps). |
| Ubicación | `expo-location` | Posición para guardar semáforos y nombre sugerido con el geocodificador del sistema (sin clave ni cuota). |
| Voz (salida) | `expo-speech` | |
| Voz (entrada, fase 3) | `expo-speech-recognition` | Reconocimiento en el dispositivo cuando esté disponible; comprobar `es-*` en cada plataforma. |
| Vibración del teléfono | `expo-haptics` | |
| Notificaciones | `expo-notifications` | Avisos cuando la app no está al frente. |
| Servicio en primer plano Android | Módulo nativo pequeño con Expo Modules (Kotlin) | Evita depender de librerías abandonadas; reutiliza ideas de `MonitoringService.kt`. |
| Estado | `zustand` | Un store con conexión, último paquete y ajustes. |
| Persistencia | `expo-sqlite` (semáforos, historial) y `expo-sqlite/kv-store` (ajustes) | |
| Siri (fase 4) | `expo-app-intents` | En alfa; sólo iOS. |
| Pruebas | Jest + `@testing-library/react-native` (consultas por rol y etiqueta) | Las consultas por accesibilidad obligan a etiquetar bien. |
| Lint | `eslint-config-expo`; falta sumar `eslint-plugin-react-native-a11y` | |

Arquitectura (lógica separada de la UI, igual que el firmware separa la lógica del
hardware para probarla en la PC):

```
app-movil/src/
  app/                 pantallas (expo-router): index (Estado), semaforos/, ajustes/
  protocolo/           UUID, comandos y parser JSON → Telemetria, tolerante a errores (probado)
  ble/                 ClienteBrazalete: real (BLE) y simulado
  brazalete/           estado compartido (zustand) y arranque del cliente
  avisos/              frases, detección de eventos sin repetir (probado) y salida
                       por lector de pantalla o síntesis de voz
  semaforos/           base SQLite, distancias y rumbos (probado), ubicación, mapa
  telemetria/          sesiones de cruce con GPS (probado), cola SQLite y envío a Loki
  (pendiente) historial/
app-movil/modules/acpi-servicio/   (pendiente) servicio en primer plano Android
```

`protocolo/` y `avisos/frases.ts` se prueban con la misma secuencia de paquetes que
reproduce el cliente simulado, como `firmware/brazalete/test/`.

### Qué se trae de la app Kotlin y qué no

| Pieza actual | Decisión |
|---|---|
| `BLEManager.kt` | Se reescribe en `src/ble/`. |
| `MonitoringService.kt` | Lógica a `src/avisos/` y `src/brazalete/` + servicio Android nativo mínimo. |
| `LocalNotificationManager.kt` (aviso "seguro cruzar", batería baja) | Pasa a `src/avisos/`, con la redacción descriptiva de la sección 2. |
| `SessionLogger.kt` + `GrafanaReporter.kt` (hecho el 29 oct 2026) | Pasan a `src/telemetria/`, sin pasos ni pánicos (el teléfono ya no cuenta pasos y el brazalete no envía pánico), con la ubicación del semáforo y la duración del cruce medida con el GPS. **Sin servidor propio**: igual que antes, el teléfono hace `POST` directo al endpoint de Loki de Grafana Cloud (`/loki/api/v1/push`, autenticación Basic `usuario:token`) con el formato de `docs/grafana-setup.md`. El token sí queda dentro de la app, así que se crea con una *access policy* que sólo tenga `logs:write`: quien lo extraiga sólo puede escribir registros, no leer ni borrar. Los envíos pendientes se guardan en SQLite y se reintentan. |
| `BLEDebugFragment.kt` | Pantalla de Diagnóstico. |
| `CompassFragment.kt` + `WalkDirectionDetector.kt` (brújula y pasos del teléfono) | **No se trae**: el rumbo que importa es el del brazalete y la brújula del teléfono en el bolsillo no es fiable. |
| Geocerca "casa" (50 m) | **No se trae** en la v1: es una función para cuidadores, con implicaciones de privacidad; se puede retomar aparte. |

---

## 8. Plan por fases

1. **Base y BLE** — proyecto Expo en `app-movil/`, `protocolo/` con pruebas,
   `BrazaleteClient`, pantalla de Diagnóstico. Hecho cuando: en un iPhone y un Android
   reales se ven los paquetes del brazalete y funcionan `TEST_ALERT`/`VIBRATE_STOP`.
   *Código hecho el 28 oct 2026 (con la pantalla Estado, avisos básicos y Semáforos);
   falta probarlo en los teléfonos.*
2. **Experiencia accesible** — niveles de aviso, Ajustes completos,
   tutorial, segundo plano en ambas plataformas, historial. Hecho cuando: con el
   teléfono bloqueado en el bolsillo y VoiceOver/TalkBack activos, se recorre un cruce
   completo con el semáforo de prueba y se escuchan los avisos correctos una sola vez.
3. **Voz** — botón "Hablar" + toque mágico, 5 comandos.
4. **Atajos del sistema** — Siri con `expo-app-intents`; equivalente Android si hay
   una vía estable.
5. **Cierre** — Grafana, actualizar `docs/ble-protocol.md` ("app móvil" en vez de
   "Android"), el README y el informe Infomatrix (sección 3.2 y capturas). La app
   Kotlin (`app/`) se retiró del repositorio el 29 oct 2026; sigue en el historial de git.

---

## 9. Cómo se prueba la accesibilidad

- Recorrer **cada pantalla con VoiceOver y con TalkBack**, con la pantalla apagada
  (cortina de pantalla en iOS: triple toque con tres dedos). Si algo no se puede hacer
  así, es un error, no una mejora pendiente.
- Accessibility Inspector (Xcode) y Accessibility Scanner (Android).
- Texto al tamaño máximo del sistema.
- Prueba en la calle con ruido real y auriculares de conducción ósea.
- **Probar con personas ciegas** (por ejemplo, a través de una asociación local) antes
  de dar el diseño por cerrado, idealmente en la fase 2. Es lo que más cambia el
  resultado, y suma mucho al informe.

---

## 10. Preguntas abiertas

- ¿Se agregará botón de pánico al brazalete (`st = 3`)? Si sí, la app debería poder
  avisar a un contacto, lo que implica permisos y diseño aparte.
- ¿Medición de batería en el hardware nuevo (`bt`)?
- ¿Hace falta soportar varios brazaletes por teléfono (pruebas del equipo)?
- Idioma: ¿sólo español, o también inglés para Infomatrix?

---

## Fuentes

- Jain et al., *Smartphone Usage by Expert Blind Users*, CHI 2021 —
  https://www.microsoft.com/en-us/research/publication/smartphone-usage-by-expert-blind-users/
- Delivery Hero, *What your mobile app looks like to someone who is blind* —
  https://tech.deliveryhero.com/what-your-mobile-app-looks-like-to-someone-who-is-blind
- W3C Mobile Accessibility Task Force, discusión sobre gestos y lectores de pantalla —
  https://www.w3.org/2016/01/21-mobile-a11y-minutes.html
- Adrian Roselli, *Mobile Assistive Technology Gesture References* —
  https://adrianroselli.com/2016/02/mobile-assistive-technology-gesture-references.html
- React Native, Accessibility (`onMagicTap`, `accessibilityLiveRegion`,
  `accessibilityActions`) — https://reactnative.dev/docs/accessibility
- OKO (AYES) — https://road-safety-charter.ec.europa.eu/content/oko-revolutionary-pedestrian-crossing-app-0
  y https://ixd.prattsi.org/2023/09/assistive-technology-oko-iphone-app/
- Microsoft Soundscape — https://www.microsoft.com/en-us/research/?p=751702 y
  https://techcrunch.com/2022/12/13/microsoft-sunsets-soundscape-3d-audio-app-but-will-open-source-the-code
- `expo-speech-recognition` — https://github.com/jamsch/expo-speech-recognition
- `expo-app-intents` — https://docs.expo.dev/versions/latest/sdk/app-intents/ y
  https://expo.dev/blog/introducing-expo-app-intents
- `react-native-ble-plx` (fork con plugin Expo y notas de segundo plano) —
  https://cdn.jsdelivr.net/npm/@sfourdrinier/react-native-ble-plx@3.9.2/docs/EXPO_PLUGIN.md
- Comparación de librerías BLE para React Native —
  https://dev.to/logrocket/comparing-react-native-ble-libraries-533p
