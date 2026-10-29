# App móvil ACPI (React Native)

## Descripción General
Reescritura en **React Native + Expo** de la antigua app Android en Kotlin, para **iOS y Android**,
pensada para personas ciegas. El diseño está en
[`docs/guia-app-invidentes.md`](../docs/guia-app-invidentes.md): funciona con VoiceOver y
TalkBack, avisa por voz los cambios importantes del brazalete sin repetirlos, y nunca da la
orden de cruzar.

Estado actual (fase 1 de la guía + semáforos + telemetría):
- Conexión BLE con el brazalete (real o **simulada**), con búsqueda y reconexión automática.
- Pantalla **Estado** con la frase del cruce, avisos de voz y "Repetir estado" (también con el
  toque mágico de VoiceOver: doble toque con dos dedos).
- **Semáforos**: guardar uno en la ubicación actual con un toque, lista ordenada por distancia
  ("a 120 metros al norte"), mapa de OpenStreetMap para marcar o corregir puntos, editar y borrar.
  Todo se guarda en el teléfono (SQLite).
- **Cruces**: cada vez que el brazalete encuentra un semáforo, el teléfono sigue el GPS, ubica el
  semáforo (y lo guarda en la lista si es nuevo), mide la espera y la duración del cruce y lo
  manda al **servidor web de ACPI en Render**. Sin red, espera en una cola local.
- **Ajustes** → modo simulado, compartir cruces, y **Diagnóstico** (datos crudos, `TEST_ALERT`,
  `VIBRATE_STOP`, envío de prueba al servidor).

Pendiente según la guía: funcionamiento en segundo plano, historial de cruces, tutorial,
niveles de aviso, comandos de voz.

---

## Contenido
- `src/app/` -> Pantallas (Expo Router). `index.tsx` es Estado; `semaforos/` y `ajustes/` tienen su propio stack.
- `src/protocolo/` -> UUID, comandos y parser del JSON del brazalete (igual a `docs/ble-protocol.md`).
- `src/ble/` -> Interfaz `ClienteBrazalete` y sus dos implementaciones: `cliente-ble.ts` (react-native-ble-manager) y `cliente-simulado.ts`.
- `src/brazalete/estado.ts` -> Estado compartido (zustand): conexión, última telemetría, registro, modo simulado.
- `src/avisos/` -> Frases, detección de qué avisar y salida por lector de pantalla o síntesis de voz.
- `src/semaforos/` -> Base local, distancias y rumbos, ubicación y el componente de mapa.
- `src/telemetria/` -> Sesiones de cruce con GPS (`sesion.ts`), cola SQLite y envío al servidor (`servidor.ts`).
- `app.config.ts` -> Configuración nativa (permisos, plugins, identificador `com.snt8.acpi`).

La lógica sin React (`protocolo/`, `avisos/frases.ts`, `semaforos/geo.ts`,
`telemetria/sesion.ts`, `telemetria/servidor.ts`) tiene pruebas en las
carpetas `__tests__/`.

---

## Explicación

### Por qué no se usa Expo Go
Expo Go sólo trae sus propios módulos nativos y esta app usa Bluetooth y MapLibre, así que
se instala una **development build** propia (compilada en la Mac). Después de instalarla una
vez, los cambios de JavaScript llegan al iPhone al instante desde Windows.

### Mapa: MapLibre + OpenFreeMap
El mapa usa [MapLibre](https://maplibre.org) (open source) con el estilo "liberty" de
[OpenFreeMap](https://openfreemap.org), que sirve datos de OpenStreetMap gratis y sin clave ni
cuenta. Es el mismo mapa en iOS y Android. Para cambiar de servidor de mapas basta otra URL de
estilo en `src/semaforos/mapa.tsx`. El mapa se oculta al lector de pantalla: la lista de
semáforos es su equivalente accesible. El nombre sugerido para cada semáforo sale del
geocodificador del sistema, que tampoco necesita clave.

### Avisos de voz
`detectarAviso` compara el estado anterior con el nuevo y sólo avisa los eventos: semáforo
detectado, cambio a verde o rojo, desconexión. La orientación no se dice por voz porque el
brazalete ya vibra hacia donde girar. Con VoiceOver/TalkBack activo el aviso va por el lector,
para no hablar encima de él.

### Cruces y telemetría
Una sesión va desde que el brazalete se conecta (sólo enciende el BLE al recibir un semáforo)
hasta que se desconecta. La ubicación del semáforo es la lectura de GPS más precisa mientras se
espera en la esquina. El cruce empieza con el primer "permitido" y termina cuando el GPS ubica a
la persona a 12 m de la esquina en la dirección del rumbo del brazalete. El detalle de cada campo
y la API que debe cumplir el servidor están en [`docs/servidor-api.md`](../docs/servidor-api.md). Por ahora
el GPS sólo se sigue con la app abierta.

---

## Dependencias
- Node 20 o superior.
- Para compilar en iOS: una Mac con Xcode, CocoaPods (`brew install cocoapods`) y un Apple ID
  (gratuito sirve; la app instalada caduca a los 7 días).
- Para Android: Android Studio, o compilar en la nube con EAS.

---

## Ejemplo de Uso

### 1. Primera instalación en el iPhone (en la Mac)
```bash
git clone https://github.com/Snt8/ACPI.git
cd ACPI/app-movil
npm install
cp .env.example .env    # URL del servidor en Render (opcional, ver docs/servidor-api.md)
npx expo run:ios --device
```
- En Xcode → Settings → Accounts tiene que estar el Apple ID. Si `run:ios` falla al firmar,
  abrir `ios/ACPI.xcworkspace`, elegir el "Personal Team" en *Signing & Capabilities* y repetir.
- En el iPhone: activar **Modo desarrollador** (Ajustes → Privacidad y seguridad) y, la primera
  vez, confiar en el certificado (Ajustes → General → VPN y gestión de dispositivos).
- Hay que volver a compilar en la Mac cuando cambia la configuración nativa (`app.config.ts`,
  una librería nueva) o cuando pasan los 7 días.

### 2. Desarrollo diario (en Windows)
```bash
cd app-movil
npm start
```
Con el iPhone en la misma Wi-Fi, abrir la app ACPI instalada: se conecta al servidor de
Windows y recarga sola al guardar cambios. La URL y el token del servidor se leen del `.env` de la
computadora que corre `npm start`, así que también hay que crearlo en Windows.

### 3. Pruebas y chequeos
```bash
npm test
npm run typecheck
npm run lint
```

### 4. Probar sin brazalete
Ajustes → **Brazalete simulado**. Reproduce un cruce completo (mal orientado → rojo → verde)
con los mismos paquetes JSON que envía el firmware.
