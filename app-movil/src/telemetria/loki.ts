/*
 * Envío directo a Grafana Cloud Loki, sin servidor propio (ver docs/grafana-setup.md).
 * Las credenciales vienen de .env (EXPO_PUBLIC_*), que Expo copia al bundle: el token
 * debe ser de una access policy con sólo `logs:write`.
 */

const RUTA_PUSH = '/loki/api/v1/push';
const TIEMPO_MAXIMO_MS = 15_000;

export interface ConfigLoki {
  url: string;
  usuario: string;
  token: string;
}

/** null si falta algún dato en .env: la telemetría se guarda en la cola y no se envía */
export function configLoki(): ConfigLoki | null {
  // Expo sólo reemplaza process.env.EXPO_PUBLIC_* escrito así, con acceso estático
  const url = process.env.EXPO_PUBLIC_LOKI_URL?.trim() ?? '';
  const usuario = process.env.EXPO_PUBLIC_LOKI_USUARIO?.trim() ?? '';
  const token = process.env.EXPO_PUBLIC_LOKI_TOKEN?.trim() ?? '';
  if (!url || !usuario || !token) {
    return null;
  }
  // Se acepta la URL base que muestra Grafana Cloud o la ruta completa de push
  const base = url.replace(/\/+$/, '');
  return { url: base.endsWith(RUTA_PUSH) ? base : base + RUTA_PUSH, usuario, token };
}

export interface EntradaLoki {
  tipo: string;
  momentoNs: string; // ns desde epoch, como texto: no cabe en un number sin perder precisión
  linea: string;
}

/** Cuerpo JSON de /loki/api/v1/push: un stream por tipo de evento, con etiquetas fijas */
export function cuerpoLoki(entradas: EntradaLoki[]): string {
  const porTipo = new Map<string, [string, string][]>();
  for (const { tipo, momentoNs, linea } of entradas) {
    porTipo.set(tipo, [...(porTipo.get(tipo) ?? []), [momentoNs, linea]]);
  }
  return JSON.stringify({
    streams: [...porTipo].map(([tipo, valores]) => ({
      stream: { app: 'acpi', tipo },
      values: valores.sort(([a], [b]) => (BigInt(a) < BigInt(b) ? -1 : 1)),
    })),
  });
}

export const momentoNs = (ms: number) => `${Math.round(ms)}000000`;

// "enviado" y "rechazado" sacan el lote de la cola (un lote mal formado nunca se va a
// aceptar); "reintentar" lo deja para después (sin red, credenciales mal, Loki caído)
export type ResultadoEnvio = { resultado: 'enviado' | 'rechazado' | 'reintentar'; detalle: string };

export async function enviarALoki(config: ConfigLoki, cuerpo: string): Promise<ResultadoEnvio> {
  const control = new AbortController();
  const temporizador = setTimeout(() => control.abort(), TIEMPO_MAXIMO_MS);
  try {
    const respuesta = await fetch(config.url, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        Authorization: `Basic ${btoa(`${config.usuario}:${config.token}`)}`,
      },
      body: cuerpo,
      signal: control.signal,
    });
    if (respuesta.ok) {
      return { resultado: 'enviado', detalle: `HTTP ${respuesta.status}` };
    }
    const detalle = `HTTP ${respuesta.status}: ${(await respuesta.text()).slice(0, 200)}`;
    const reintentar = respuesta.status === 401 || respuesta.status === 403 || respuesta.status === 429 || respuesta.status >= 500;
    return { resultado: reintentar ? 'reintentar' : 'rechazado', detalle };
  } catch (error) {
    return { resultado: 'reintentar', detalle: `Sin conexión con Loki: ${String(error)}` };
  } finally {
    clearTimeout(temporizador);
  }
}
