// Protocolo BLE del brazalete. Debe coincidir con
// firmware/brazalete/src/comunicacion/movil/movil.cpp y docs/ble-protocol.md.

export const NOMBRE_BRAZALETE = 'ACPI_Brazalete';
export const UUID_SERVICIO = '12345678-1234-1234-1234-123456789abc';
export const UUID_TX = '87654321-4321-4321-4321-cba987654321'; // notify: brazalete → app
export const UUID_RX = '11111111-2222-3333-4444-555555555555'; // write: app → brazalete

// Comandos de diagnóstico que acepta la característica RX
export type Comando = 'TEST_ALERT' | 'VIBRATE_STOP';

// Valores de "st" tal como los envía hoy el firmware (fsm.cpp, enviarTelemetria).
// "panico" (3) está documentado pero el brazalete aún no lo envía.
export type EstadoCruce = 'prohibido' | 'permitido' | 'mal-orientado' | 'panico' | 'desconocido';

const ESTADOS: Record<number, EstadoCruce> = {
  0: 'prohibido',
  1: 'permitido',
  2: 'mal-orientado',
  3: 'panico',
};

export interface Telemetria {
  rumbo: number; // 0-359°, orientación del brazalete
  estado: EstadoCruce;
  orientado: boolean;
  segundosRestantes: number; // en el estado actual del semáforo
  bateria: number | null; // %, null si el brazalete no la mide (bt = -1)
}

function esEntero(valor: unknown): valor is number {
  return typeof valor === 'number' && Number.isInteger(valor);
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
  const { hd, st, ok, tr, bt } = datos as Record<string, unknown>;
  if (!esEntero(st)) {
    return null;
  }

  return {
    rumbo: esEntero(hd) ? ((hd % 360) + 360) % 360 : 0,
    estado: ESTADOS[st] ?? 'desconocido',
    orientado: ok === true,
    segundosRestantes: esEntero(tr) ? Math.min(Math.max(tr, 0), 255) : 0,
    bateria: esEntero(bt) && bt >= 0 && bt <= 100 ? bt : null,
  };
}

// El protocolo es ASCII, así que basta con un byte por carácter (Hermes no
// garantiza TextDecoder en todas las versiones)
export function bytesATexto(bytes: readonly number[]): string {
  return String.fromCharCode(...bytes);
}

export function textoABytes(texto: string): number[] {
  return Array.from(texto, (c) => c.charCodeAt(0) & 0xff);
}
