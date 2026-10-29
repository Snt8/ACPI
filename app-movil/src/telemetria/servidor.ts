/*
 * Envío al servidor web de ACPI, alojado en Render. El contrato está en
 * docs/servidor-api.md. La URL y el token vienen de .env (EXPO_PUBLIC_*), que Expo
 * copia al bundle: el token no es secreto y sólo debe permitir escribir eventos.
 */

const RUTA_EVENTOS = '/api/eventos';
const RUTA_SALUD = '/api/salud';
// El plan gratis de Render apaga el servidor tras unos minutos sin uso y la primera
// petición lo despierta, lo que puede tardar cerca de un minuto
const TIEMPO_MAXIMO_MS = 90_000;

export interface ConfigServidor {
  url: string; // base, sin barra final
  token: string; // vacío si el servidor no pide autenticación
}

/** null si falta la URL en .env: los eventos esperan en la cola y no se envían */
export function configServidor(): ConfigServidor | null {
  // Expo sólo reemplaza process.env.EXPO_PUBLIC_* escrito así, con acceso estático
  const url = process.env.EXPO_PUBLIC_API_URL?.trim() ?? '';
  const token = process.env.EXPO_PUBLIC_API_TOKEN?.trim() ?? '';
  return url ? { url: url.replace(/\/+$/, ''), token } : null;
}

/** Cuerpo de POST /api/eventos a partir de los eventos guardados en la cola (ya en JSON) */
export function cuerpoEventos(eventos: string[]): string {
  return `{"eventos":[${eventos.join(',')}]}`;
}

// "enviado" y "rechazado" sacan el lote de la cola (uno mal formado nunca se va a
// aceptar); "reintentar" lo deja para después (sin red, token mal, servidor caído)
export type ResultadoEnvio = { resultado: 'enviado' | 'rechazado' | 'reintentar'; detalle: string };

const RECHAZO_DEFINITIVO = new Set([400, 413, 422]);

function cabeceras(config: ConfigServidor): Record<string, string> {
  return config.token ? { Authorization: `Bearer ${config.token}` } : {};
}

async function conTiempoMaximo<T>(peticion: (signal: AbortSignal) => Promise<T>): Promise<T> {
  const control = new AbortController();
  const temporizador = setTimeout(() => control.abort(), TIEMPO_MAXIMO_MS);
  try {
    return await peticion(control.signal);
  } finally {
    clearTimeout(temporizador);
  }
}

export async function enviarEventos(config: ConfigServidor, cuerpo: string): Promise<ResultadoEnvio> {
  try {
    const respuesta = await conTiempoMaximo((signal) =>
      fetch(config.url + RUTA_EVENTOS, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json', ...cabeceras(config) },
        body: cuerpo,
        signal,
      }),
    );
    if (respuesta.ok) {
      return { resultado: 'enviado', detalle: `HTTP ${respuesta.status}` };
    }
    const detalle = `HTTP ${respuesta.status}: ${(await respuesta.text()).slice(0, 200)}`;
    return { resultado: RECHAZO_DEFINITIVO.has(respuesta.status) ? 'rechazado' : 'reintentar', detalle };
  } catch (error) {
    return { resultado: 'reintentar', detalle: `Sin conexión con el servidor: ${String(error)}` };
  }
}

/**
 * Despierta el servidor sin esperar respuesta. Se llama al detectar un semáforo, para
 * que esté listo cuando termine el cruce.
 */
export function despertarServidor(config: ConfigServidor) {
  void conTiempoMaximo((signal) => fetch(config.url + RUTA_SALUD, { signal })).catch(() => undefined);
}
