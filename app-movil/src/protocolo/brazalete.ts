// Protocolo BLE del brazalete. Debe coincidir con
// firmware/brazalete/src/comunicacion/movil/paquete_telemetria.cpp y docs/ble-protocol.md.

export const NOMBRE_BRAZALETE = 'ACPI_Brazalete';
export const UUID_SERVICIO = '12345678-1234-1234-1234-123456789abc';
export const UUID_TX = '87654321-4321-4321-4321-cba987654321'; // notify: brazalete → app
export const UUID_RX = '11111111-2222-3333-4444-555555555555'; // write: app → brazalete

// Comandos de diagnóstico que acepta la característica RX. El brazalete los
// atiende respetando la separación de 3 s entre patrones (patrones.md).
export type Comando = 'TEST_ALERT' | 'VIBRATE_STOP';

// Valores de "st" tal como los envía el firmware (fsm.cpp, enviarTelemetria).
// "panico" (3) está documentado pero el brazalete aún no lo envía.
export type EstadoCruce = 'prohibido' | 'permitido' | 'mal-orientado' | 'panico' | 'desconocido';

const ESTADOS: Record<number, EstadoCruce> = {
  0: 'prohibido',
  1: 'permitido',
  2: 'mal-orientado',
  3: 'panico',
};

// "br": si la brújula del brazalete sirve
export type EstadoBrujula = 'sin-calibrar' | 'valida' | 'descartada';
const BRUJULAS: Record<number, EstadoBrujula> = { 0: 'sin-calibrar', 1: 'valida', 2: 'descartada' };

// "pt": patrón que vibra en ese momento (enum Patron de controlador_patrones.h)
export type PatronVibracion =
  | 'ninguno'
  | 'detectado'
  | 'girar-derecha'
  | 'girar-izquierda'
  | 'permitido'
  | 'prohibido';
const PATRONES: Record<number, PatronVibracion> = {
  0: 'ninguno',
  1: 'detectado',
  2: 'girar-derecha',
  3: 'girar-izquierda',
  4: 'permitido',
  5: 'prohibido',
};

/**
 * Campos que agregó la versión 2 del paquete (firmware del 29 oct 2026). Cada uno es
 * null si no llegó o vino fuera de rango.
 */
export interface TelemetriaExtendida {
  secuencia: number | null; // "n": sube 1 por paquete (0-65535); un salto = paquetes perdidos
  semaforoId: number | null; // "id" del semáforo que sigue el brazalete
  verde: boolean | null; // "lz": luz peatonal, aunque la persona esté mal orientada
  rumboSemaforo: number | null; // "hs": 0-359°
  giro: number | null; // "gr": grados para quedar de frente, -180..180 (+ derecha)
  brujula: EstadoBrujula | null; // "br"
  patron: PatronVibracion | null; // "pt"
  paquetesSemaforo: number | null; // "rx": paquetes del semáforo en los últimos 10 s (20 = todos)
  rssi: number | null; // "rs": dBm del último paquete del semáforo
}

export interface Telemetria {
  rumbo: number; // 0-359°, orientación del brazalete
  estado: EstadoCruce;
  orientado: boolean;
  segundosRestantes: number; // en el estado actual del semáforo
  bateria: number | null; // %, null si el brazalete no la mide (bt = -1)
  // Sólo con firmware que envía "v" ≥ 2; sin ella la app funciona igual que antes
  extendida?: TelemetriaExtendida;
}

function esEntero(valor: unknown): valor is number {
  return typeof valor === 'number' && Number.isInteger(valor);
}

const enRango = (valor: unknown, min: number, max: number): number | null =>
  esEntero(valor) && valor >= min && valor <= max ? valor : null;

function extendida(datos: Record<string, unknown>): TelemetriaExtendida {
  const { n, id, lz, hs, gr, br, pt, rx, rs } = datos;
  const rssi = enRango(rs, -127, -1); // 0 = el brazalete no tiene el dato
  return {
    secuencia: enRango(n, 0, 65535),
    semaforoId: enRango(id, 0, 255),
    verde: lz === 1 ? true : lz === 0 ? false : null,
    rumboSemaforo: enRango(hs, 0, 359),
    giro: enRango(gr, -180, 180),
    brujula: esEntero(br) ? (BRUJULAS[br] ?? null) : null,
    patron: esEntero(pt) ? (PATRONES[pt] ?? null) : null,
    paquetesSemaforo: enRango(rx, 0, 255),
    rssi,
  };
}

/**
 * Convierte el JSON de una notificación en Telemetria. Devuelve null si el texto no
 * es un paquete válido (JSON partido, sin "st"…) para que quien llame lo descarte sin
 * cortar la conexión. Los campos opcionales que falten o estén fuera de rango toman
 * valores neutros.
 */
export function parsearTelemetria(texto: string): Telemetria | null {
  let datos: unknown;
  try {
    datos = JSON.parse(texto);
  } catch {
    return null;
  }
  if (typeof datos !== 'object' || datos === null) {
    return null;
  }
  const campos = datos as Record<string, unknown>;
  const { v, hd, st, ok, tr, bt } = campos;
  if (!esEntero(st)) {
    return null;
  }

  const telemetria: Telemetria = {
    rumbo: esEntero(hd) ? ((hd % 360) + 360) % 360 : 0,
    estado: ESTADOS[st] ?? 'desconocido',
    orientado: ok === true,
    segundosRestantes: esEntero(tr) ? Math.min(Math.max(tr, 0), 255) : 0,
    bateria: esEntero(bt) && bt >= 0 && bt <= 100 ? bt : null,
  };
  if (esEntero(v) && v >= 2) {
    telemetria.extendida = extendida(campos);
  }
  return telemetria;
}

// El protocolo es ASCII, así que basta con un byte por carácter (Hermes no
// garantiza TextDecoder en todas las versiones)
export function bytesATexto(bytes: readonly number[]): string {
  return String.fromCharCode(...bytes);
}

export function textoABytes(texto: string): number[] {
  return Array.from(texto, (c) => c.charCodeAt(0) & 0xff);
}
