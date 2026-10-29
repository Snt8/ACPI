// Lo que la app dice en voz alta. Las frases describen el estado; nunca ordenan
// cruzar (ver docs/guia-app-invidentes.md, sección 2).

import type { EstadoConexion } from '@/ble/cliente';
import type { EstadoCruce, Telemetria } from '@/protocolo/brazalete';

export type Prioridad = 'alta' | 'normal';

export interface Aviso {
  texto: string;
  prioridad: Prioridad;
}

export interface Instantanea {
  conexion: EstadoConexion;
  telemetria: Telemetria | null;
}

const FRASES_CONEXION: Record<EstadoConexion, string> = {
  apagado: 'Conexión con el brazalete detenida',
  'sin-permisos': 'Falta permiso de Bluetooth. Actívalo en los ajustes del teléfono',
  'bluetooth-apagado': 'El Bluetooth del teléfono está apagado',
  buscando: 'Buscando el brazalete',
  conectando: 'Conectando con el brazalete',
  conectado: 'Brazalete conectado',
};

function segundos(n: number): string {
  return n === 1 ? '1 segundo' : `${n} segundos`;
}

function fraseCruce(t: Telemetria): string {
  switch (t.estado) {
    case 'permitido':
      return `Paso peatonal en verde, quedan ${segundos(t.segundosRestantes)}`;
    case 'prohibido':
      return `Paso peatonal en rojo, quedan ${segundos(t.segundosRestantes)}`;
    case 'mal-orientado':
      return 'No estás de frente al cruce. El brazalete vibra hacia donde girar';
    case 'panico':
      return 'Alerta de pánico activada';
    default:
      return 'Estado del semáforo desconocido';
  }
}

/** Frase completa del estado actual, para la tarjeta de estado y "Repetir estado" */
export function fraseEstado({ conexion, telemetria }: Instantanea): string {
  if (conexion !== 'conectado') {
    return FRASES_CONEXION[conexion];
  }
  if (!telemetria) {
    return 'Brazalete conectado, esperando datos del semáforo';
  }
  return fraseCruce(telemetria);
}

// Estados que se anuncian solos al cambiar. "mal-orientado" no: el brazalete ya
// vibra hacia donde girar cada 3 s, y la voz encima sólo estorbaría.
const CRUCE_ANUNCIADO: ReadonlySet<EstadoCruce> = new Set(['permitido', 'prohibido', 'panico']);

/**
 * Decide si un cambio entre dos instantáneas merece un aviso de voz. Cada evento se
 * avisa una sola vez, igual que el brazalete vibra una vez por evento: los paquetes
 * repetidos (cada 500 ms) no generan avisos.
 */
export function detectarAviso(anterior: Instantanea, actual: Instantanea): Aviso | null {
  if (anterior.conexion !== actual.conexion) {
    if (actual.conexion === 'conectado') {
      // El brazalete sólo se conecta cuando oyó a un semáforo
      return { texto: 'Semáforo detectado', prioridad: 'normal' };
    }
    if (anterior.conexion === 'conectado') {
      return { texto: 'Brazalete desconectado', prioridad: 'alta' };
    }
    if (actual.conexion === 'sin-permisos' || actual.conexion === 'bluetooth-apagado') {
      return { texto: FRASES_CONEXION[actual.conexion], prioridad: 'normal' };
    }
    return null;
  }

  const antes = anterior.telemetria?.estado;
  const ahora = actual.telemetria;
  if (actual.conexion !== 'conectado' || !ahora || ahora.estado === antes) {
    return null;
  }
  if (!CRUCE_ANUNCIADO.has(ahora.estado)) {
    return null;
  }
  // Al volver a quedar de frente tras estar mal orientada se repite el color aunque
  // no haya cambiado: es justo cuando la persona necesita saberlo
  return { texto: fraseCruce(ahora), prioridad: ahora.estado === 'panico' ? 'alta' : 'normal' };
}
