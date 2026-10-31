// Decisiones del cliente BLE que no dependen de la librería ni del teléfono: qué hacer
// con cada estado del adaptador, a qué periférico conectarse y cuánto esperar entre
// búsquedas. Son funciones puras para poder probarlas en el PC (__tests__/decisiones.test.ts).

import type { Permission } from 'react-native';
import type { BleState, Peripheral } from 'react-native-ble-manager';

import { NOMBRE_BRAZALETE, UUID_SERVICIO, mismoUuid } from '@/protocolo/brazalete';

// --- Adaptador -------------------------------------------------------------------

export type EstadoAdaptador = `${BleState}`;

export type ReaccionAdaptador = 'listo' | 'apagado' | 'sin-permisos' | 'no-compatible' | 'esperar';

/** Qué hacer con cada estado que informa el Bluetooth del teléfono */
export function reaccionAlAdaptador(estado: string): ReaccionAdaptador {
  switch (estado) {
    case 'on':
      return 'listo';
    case 'off':
      return 'apagado';
    case 'unauthorized':
      return 'sin-permisos';
    case 'unsupported':
      return 'no-compatible';
    default:
      // unknown (iOS al arrancar), resetting, turning_on y turning_off duran poco y
      // después llega otro estado: anunciar "Bluetooth apagado" sería falso
      return 'esperar';
  }
}

// --- Permisos (Android) ----------------------------------------------------------

/**
 * Permisos que hay que pedir en Android. Desde Android 12 (API 31) BLUETOOTH_SCAN se
 * declara con neverForLocation (app.config.ts), así que la ubicación no hace falta para
 * buscar el brazalete; la pide aparte expo-location, para el GPS.
 */
export function permisosAndroid(versionApi: number): Permission[] {
  return versionApi >= 31
    ? ['android.permission.BLUETOOTH_SCAN', 'android.permission.BLUETOOTH_CONNECT']
    : ['android.permission.ACCESS_FINE_LOCATION'];
}

// --- Periféricos -----------------------------------------------------------------

// iOS guarda el nombre GAP de la primera conexión y lo sigue mostrando aunque el
// firmware lo cambie; "ACPI_Pulsera" es el nombre que tenían los primeros firmwares
const NOMBRES_CONOCIDOS: readonly string[] = [NOMBRE_BRAZALETE, 'ACPI_Pulsera'];

export type Veredicto = { aceptar: true; nota: string | null } | { aceptar: false; motivo: string };

/** El nombre del advertising es el actual; el de `name` puede venir de la caché del sistema */
export const nombreDe = (p: Peripheral): string | null => p.advertising?.localName ?? p.name ?? null;

const describir = (nombre: string | null) => (nombre ? `"${nombre}"` : 'sin nombre');

/**
 * Para un periférico que encontró el escaneo. El escaneo ya filtra por el UUID del
 * servicio (lo hace el sistema), así que se acepta aunque el nombre no coincida; sólo
 * se descarta si su advertising dice explícitamente que no tiene el servicio.
 */
export function evaluarEncontrado(p: Peripheral): Veredicto {
  const nombre = nombreDe(p);
  const servicios = p.advertising?.serviceUUIDs ?? [];
  const anunciaServicio = servicios.some((s) => mismoUuid(s, UUID_SERVICIO));
  if (servicios.length > 0 && !anunciaServicio && !(nombre && NOMBRES_CONOCIDOS.includes(nombre))) {
    return { aceptar: false, motivo: `${describir(nombre)} no anuncia el servicio del brazalete` };
  }
  return { aceptar: true, nota: nombre === NOMBRE_BRAZALETE ? null : `se anuncia como ${describir(nombre)}` };
}

/**
 * Para un periférico que ya estaba conectado al arrancar (por ejemplo, tras recargar el
 * JS). iOS sólo devuelve los que tienen el servicio; Android devuelve todo lo conectado
 * al teléfono (reloj, auriculares…), así que ahí decide el nombre.
 */
export function evaluarConectado(p: Peripheral, plataforma: string): Veredicto {
  if (plataforma === 'ios') {
    return { aceptar: true, nota: null };
  }
  const nombre = p.name ?? p.advertising?.localName ?? null;
  return nombre && NOMBRES_CONOCIDOS.includes(nombre)
    ? { aceptar: true, nota: null }
    : { aceptar: false, motivo: `${describir(nombre)} no es el brazalete` };
}

// --- Búsqueda y reintentos -------------------------------------------------------

export interface PlanBusqueda {
  segundos: number;
  rapida: boolean; // Android: scanMode LowLatency en vez de LowPower
}

/**
 * Con la app al frente se busca sin pausas y a máxima velocidad: el brazalete sólo
 * aparece cerca de un semáforo y cada segundo cuenta. La búsqueda se reinicia cada
 * 30 s (el sistema deja de reportar un equipo que ya vio en esa búsqueda). En segundo
 * plano, búsquedas largas de bajo consumo: iOS no corre temporizadores de JS ahí, pero
 * la búsqueda filtrada por servicio sigue y despierta a la app al encontrarlo.
 */
export function planBusqueda(primerPlano: boolean): PlanBusqueda {
  return primerPlano ? { segundos: 30, rapida: true } : { segundos: 300, rapida: false };
}

// Android deja empezar 5 búsquedas cada 30 s por app; la sexta no da error, sólo no
// encuentra nada. Se deja un segundo de margen.
export const LIMITE_BUSQUEDAS_ANDROID = 5;
export const VENTANA_LIMITE_MS = 31_000;

/** ms que hay que esperar para empezar otra búsqueda sin pasar el límite de Android */
export function esperaPorLimite(inicios: readonly number[], ahora: number): number {
  const recientes = inicios.filter((t) => ahora - t < VENTANA_LIMITE_MS).sort((a, b) => a - b);
  if (recientes.length < LIMITE_BUSQUEDAS_ANDROID) {
    return 0;
  }
  return recientes[recientes.length - LIMITE_BUSQUEDAS_ANDROID] + VENTANA_LIMITE_MS - ahora;
}

/** Agrega un inicio y descarta los que ya salieron de la ventana */
export function anotarInicio(inicios: readonly number[], ahora: number): number[] {
  return [...inicios.filter((t) => ahora - t < VENTANA_LIMITE_MS), ahora];
}

// Sólo tras un error (búsqueda que no arranca, conexión fallida). Una búsqueda que
// termina sin encontrar nada no es un error: se vuelve a buscar enseguida.
const ESPERAS_TRAS_FALLO_MS = [2000, 5000, 10000, 30000];

/** Espera antes del siguiente intento tras `fallos` errores seguidos (1 o más) */
export function esperaTrasFallo(fallos: number): number {
  const i = Math.min(Math.max(fallos, 1), ESPERAS_TRAS_FALLO_MS.length) - 1;
  return ESPERAS_TRAS_FALLO_MS[i];
}
