import type { EstadoCruce, Telemetria } from '@/protocolo/brazalete';
import { diferenciaAngular, distanciaMetros, rumboGrados } from '@/semaforos/geo';
import type { Posicion } from '@/semaforos/ubicacion';

/*
 * Una sesión va desde que el brazalete encuentra un semáforo (se conecta: el firmware
 * sólo enciende el BLE al recibir paquetes ESP-NOW) hasta que se desconecta (30 s sin
 * semáforo). Mientras dura, el teléfono sigue el GPS:
 *
 * - La ubicación del semáforo es la del teléfono en la esquina, mientras espera el verde.
 *   Se queda la lectura más precisa de ese tramo, porque la persona está quieta.
 * - El cruce empieza con el primer paquete "permitido" (verde y bien orientado). El
 *   rumbo del brazalete en ese momento es hacia dónde cruza.
 * - El cruce termina cuando el GPS la ubica a DISTANCIA_CRUCE_M de la esquina y en esa
 *   dirección. Si se aleja por la misma acera, no cuenta como cruce.
 *
 * Con firmware que envía la telemetría extendida (versión 2 del paquete BLE) se miden
 * además los enlaces: paquetes BLE perdidos (saltos en "n"), recepción de los paquetes
 * del semáforo ("rx", de 20 cada 10 s) y su señal ("rs", el alcance).
 */

// Ancho típico de una calle de dos carriles más la acera; en avenidas anchas el cruce
// se da por terminado antes de llegar al otro lado
export const DISTANCIA_CRUCE_M = 12;
// El rumbo del brazalete es magnético y el del GPS es geográfico: la tolerancia cubre
// la declinación y el ruido del GPS
export const TOLERANCIA_RUMBO_GRADOS = 60;
// Con más error que esto, 12 m de desplazamiento pueden ser puro ruido
export const PRECISION_MAXIMA_M = 25;
// El semáforo manda 2 paquetes por segundo y el brazalete cuenta los de 10 s
export const PAQUETES_SEMAFORO_ESPERADOS = 20;
// Un salto mayor en "n" no son paquetes perdidos sino un brazalete reiniciado
const SALTO_MAXIMO_SECUENCIA = 1000;

export interface SesionCruce {
  id: string;
  inicio: number; // ms desde epoch
  esquina: Posicion | null;
  inicioVerde: number | null;
  rumboCruce: number | null;
  verdeRestante: number | null; // segundos de verde que quedaban al empezar el cruce
  finCruce: number | null;
  vecesMalOrientado: number;
  ultimoEstado: EstadoCruce | null;
  // Enlaces (telemetría extendida)
  paquetesBle: number;
  paquetesBlePerdidos: number;
  ultimaSecuencia: number | null;
  sumaRecepcion: number; // suma de "rx"
  muestrasRecepcion: number;
  sumaRssi: number;
  muestrasRssi: number;
  rssiMinimo: number | null;
  semaforoBleId: number | null;
  sinCalibrar: boolean; // el brazalete avisó la brújula sin calibrar
}

/** Lo que se manda al servidor al terminar la sesión (tipo "cruce") */
export interface ResumenCruce {
  sesion: string;
  inicio: number;
  lat: number | null;
  lon: number | null;
  precision_m: number | null;
  espera_s: number | null;
  cruzo: boolean;
  duracion_cruce_s: number | null;
  verde_restante_s: number | null;
  rumbo_cruce: number | null;
  veces_mal_orientado: number;
  duracion_sesion_s: number;
  // Enlaces: null con firmware sin telemetría extendida
  paquetes_ble: number;
  paquetes_ble_perdidos: number | null;
  recepcion_semaforo_pct: number | null;
  rssi_semaforo_medio_dbm: number | null;
  rssi_semaforo_min_dbm: number | null;
  semaforo_ble_id: number | null;
  brujula_sin_calibrar: boolean;
}

export function nuevaSesion(id: string, ahora: number): SesionCruce {
  return {
    id,
    inicio: ahora,
    esquina: null,
    inicioVerde: null,
    rumboCruce: null,
    verdeRestante: null,
    finCruce: null,
    vecesMalOrientado: 0,
    ultimoEstado: null,
    paquetesBle: 0,
    paquetesBlePerdidos: 0,
    ultimaSecuencia: null,
    sumaRecepcion: 0,
    muestrasRecepcion: 0,
    sumaRssi: 0,
    muestrasRssi: 0,
    rssiMinimo: null,
    semaforoBleId: null,
    sinCalibrar: false,
  };
}

export function conPaquete(sesion: SesionCruce, t: Telemetria, ahora: number): SesionCruce {
  const s = { ...sesion, ultimoEstado: t.estado, paquetesBle: sesion.paquetesBle + 1 };
  const ext = t.extendida;
  if (ext) {
    if (ext.secuencia !== null) {
      if (sesion.ultimaSecuencia !== null) {
        const salto = (ext.secuencia - sesion.ultimaSecuencia + 65536) % 65536;
        if (salto > 1 && salto <= SALTO_MAXIMO_SECUENCIA) {
          s.paquetesBlePerdidos += salto - 1;
        }
      }
      s.ultimaSecuencia = ext.secuencia;
    }
    if (ext.paquetesSemaforo !== null) {
      s.sumaRecepcion += Math.min(ext.paquetesSemaforo, PAQUETES_SEMAFORO_ESPERADOS);
      s.muestrasRecepcion++;
    }
    if (ext.rssi !== null) {
      s.sumaRssi += ext.rssi;
      s.muestrasRssi++;
      s.rssiMinimo = s.rssiMinimo === null ? ext.rssi : Math.min(s.rssiMinimo, ext.rssi);
    }
    s.semaforoBleId = ext.semaforoId ?? s.semaforoBleId;
    s.sinCalibrar = s.sinCalibrar || ext.brujula === 'sin-calibrar';
  }
  if (t.estado === 'mal-orientado' && sesion.ultimoEstado !== 'mal-orientado') {
    s.vecesMalOrientado++;
  }
  if (t.estado === 'permitido' && sesion.inicioVerde === null) {
    s.inicioVerde = ahora;
    s.rumboCruce = t.rumbo;
    s.verdeRestante = t.segundosRestantes;
  }
  return s;
}

const error = (p: Posicion) => p.precisionMetros ?? Infinity;

export function conPosicion(sesion: SesionCruce, p: Posicion, ahora: number): SesionCruce {
  const { esquina } = sesion;
  // Esperando el verde (o primera lectura): quedarse con la más precisa
  if (!esquina || (sesion.inicioVerde === null && error(p) < error(esquina))) {
    return { ...sesion, esquina: p };
  }
  if (sesion.inicioVerde === null || sesion.finCruce !== null || error(p) > PRECISION_MAXIMA_M) {
    return sesion;
  }
  const lejos = distanciaMetros(esquina, p) >= DISTANCIA_CRUCE_M;
  const haciaElCruce =
    sesion.rumboCruce === null ||
    diferenciaAngular(rumboGrados(esquina, p), sesion.rumboCruce) <= TOLERANCIA_RUMBO_GRADOS;
  return lejos && haciaElCruce ? { ...sesion, finCruce: ahora } : sesion;
}

const segundos = (ms: number) => Math.round(ms / 100) / 10;

export function resumir(sesion: SesionCruce, ahora: number): ResumenCruce {
  const { esquina, inicioVerde, finCruce } = sesion;
  return {
    sesion: sesion.id,
    inicio: sesion.inicio,
    lat: esquina?.latitud ?? null,
    lon: esquina?.longitud ?? null,
    precision_m: esquina?.precisionMetros == null ? null : Math.round(esquina.precisionMetros),
    espera_s: inicioVerde === null ? null : segundos(inicioVerde - sesion.inicio),
    cruzo: finCruce !== null,
    duracion_cruce_s: finCruce !== null && inicioVerde !== null ? segundos(finCruce - inicioVerde) : null,
    verde_restante_s: sesion.verdeRestante,
    rumbo_cruce: sesion.rumboCruce,
    veces_mal_orientado: sesion.vecesMalOrientado,
    duracion_sesion_s: segundos(ahora - sesion.inicio),
    paquetes_ble: sesion.paquetesBle,
    paquetes_ble_perdidos: sesion.ultimaSecuencia === null ? null : sesion.paquetesBlePerdidos,
    recepcion_semaforo_pct:
      sesion.muestrasRecepcion === 0
        ? null
        : Math.round((100 * sesion.sumaRecepcion) / (sesion.muestrasRecepcion * PAQUETES_SEMAFORO_ESPERADOS)),
    rssi_semaforo_medio_dbm: sesion.muestrasRssi === 0 ? null : Math.round(sesion.sumaRssi / sesion.muestrasRssi),
    rssi_semaforo_min_dbm: sesion.rssiMinimo,
    semaforo_ble_id: sesion.semaforoBleId,
    brujula_sin_calibrar: sesion.sinCalibrar,
  };
}
