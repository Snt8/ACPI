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
 */

// Ancho típico de una calle de dos carriles más la acera; en avenidas anchas el cruce
// se da por terminado antes de llegar al otro lado
export const DISTANCIA_CRUCE_M = 12;
// El rumbo del brazalete es magnético y el del GPS es geográfico: la tolerancia cubre
// la declinación y el ruido del GPS
export const TOLERANCIA_RUMBO_GRADOS = 60;
// Con más error que esto, 12 m de desplazamiento pueden ser puro ruido
export const PRECISION_MAXIMA_M = 25;

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
}

/** Lo que se manda a Loki al terminar la sesión (tipo "cruce") */
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
  };
}

export function conPaquete(sesion: SesionCruce, t: Telemetria, ahora: number): SesionCruce {
  const s = { ...sesion, ultimoEstado: t.estado };
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
  };
}
