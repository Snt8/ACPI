import type { SQLiteDatabase } from 'expo-sqlite';
import Storage from 'expo-sqlite/kv-store';
import { create } from 'zustand';

import { configLoki, cuerpoLoki, enviarALoki, momentoNs } from '@/telemetria/loki';

/*
 * Cola de eventos para Loki en SQLite (tabla envios_pendientes, ver repositorio.ts).
 * Todo evento pasa por aquí: si no hay red o faltan credenciales, espera en el
 * teléfono y se envía después. La telemetría nunca debe interrumpir los avisos, así
 * que ningún error sale de este módulo.
 */

export type TipoEvento = 'cruce' | 'semaforo' | 'prueba';

const VERSION_FORMATO = 2; // la 1 era el JSON de la app Kotlin
const CLAVE_ACTIVA = 'telemetria.activa';
const CLAVE_DISPOSITIVO = 'telemetria.dispositivo';
const MAXIMO_PENDIENTES = 1000; // sin red por semanas: se descartan los más viejos
const LOTE = 100;

interface EstadoTelemetria {
  activa: boolean;
  configurada: boolean;
  pendientes: number;
  ultimoEnvio: string | null; // texto para Diagnóstico
}

export const useEstadoTelemetria = create<EstadoTelemetria>(() => ({
  activa: Storage.getItemSync(CLAVE_ACTIVA) !== 'false',
  configurada: configLoki() !== null,
  pendientes: 0,
  ultimoEnvio: null,
}));

/** Identificador corto y aleatorio; no sale del teléfono más que como seudónimo */
export function idAleatorio(): string {
  return Date.now().toString(36) + Math.random().toString(36).slice(2, 10);
}

/** Seudónimo del teléfono, para separar los datos de cada usuario sin saber quién es */
function dispositivo(): string {
  let id = Storage.getItemSync(CLAVE_DISPOSITIVO);
  if (!id) {
    id = idAleatorio();
    Storage.setItemSync(CLAVE_DISPOSITIVO, id);
  }
  return id;
}

const oyentes = new Set<() => void>();

/** Avisa cada vez que entra un evento, para enviarlo sin esperar al siguiente ciclo */
export function alEncolar(oyente: () => void) {
  oyentes.add(oyente);
  return () => {
    oyentes.delete(oyente);
  };
}

export async function contarPendientes(db: SQLiteDatabase) {
  const fila = await db.getFirstAsync<{ n: number }>('SELECT COUNT(*) AS n FROM envios_pendientes');
  useEstadoTelemetria.setState({ pendientes: fila?.n ?? 0 });
}

export async function encolar(db: SQLiteDatabase, tipo: TipoEvento, datos: Record<string, unknown>) {
  if (!useEstadoTelemetria.getState().activa) {
    return;
  }
  const ahora = Date.now();
  const linea = JSON.stringify({ v: VERSION_FORMATO, tipo, dispositivo: dispositivo(), momento: ahora, ...datos });
  try {
    await db.runAsync(
      'INSERT INTO envios_pendientes (tipo, momento_ns, linea) VALUES (?, ?, ?)',
      tipo,
      momentoNs(ahora),
      linea,
    );
    await db.runAsync(
      `DELETE FROM envios_pendientes
       WHERE id NOT IN (SELECT id FROM envios_pendientes ORDER BY id DESC LIMIT ?)`,
      MAXIMO_PENDIENTES,
    );
    await contarPendientes(db);
    oyentes.forEach((oyente) => oyente());
  } catch (error) {
    useEstadoTelemetria.setState({ ultimoEnvio: `No se pudo guardar el evento: ${String(error)}` });
  }
}

interface FilaPendiente {
  id: number;
  tipo: string;
  momento_ns: string;
  linea: string;
}

let enviando = false;

/** Manda a Loki todo lo pendiente, en lotes; lo que no se pudo enviar queda en la cola */
export async function vaciarCola(db: SQLiteDatabase) {
  const config = configLoki();
  if (enviando || !config) {
    return;
  }
  enviando = true;
  try {
    for (;;) {
      const filas = await db.getAllAsync<FilaPendiente>(
        'SELECT id, tipo, momento_ns, linea FROM envios_pendientes ORDER BY id LIMIT ?',
        LOTE,
      );
      if (filas.length === 0) {
        break;
      }
      const cuerpo = cuerpoLoki(filas.map((f) => ({ tipo: f.tipo, momentoNs: f.momento_ns, linea: f.linea })));
      const { resultado, detalle } = await enviarALoki(config, cuerpo);
      const hora = new Date().toLocaleTimeString();
      useEstadoTelemetria.setState({
        ultimoEnvio: `${hora}  ${resultado === 'enviado' ? `${filas.length} eventos enviados` : detalle}`,
      });
      if (resultado === 'reintentar') {
        break;
      }
      await db.runAsync(`DELETE FROM envios_pendientes WHERE id IN (${filas.map(() => '?').join(',')})`, ...filas.map((f) => f.id));
    }
  } catch (error) {
    useEstadoTelemetria.setState({ ultimoEnvio: `Error al enviar: ${String(error)}` });
  } finally {
    enviando = false;
    await contarPendientes(db).catch(() => undefined);
  }
}

/** Al desactivarla se borra lo pendiente: lo que el usuario no quiere compartir no sale */
export async function activarTelemetria(db: SQLiteDatabase, activa: boolean) {
  Storage.setItemSync(CLAVE_ACTIVA, String(activa));
  useEstadoTelemetria.setState({ activa });
  if (!activa) {
    await db.runAsync('DELETE FROM envios_pendientes');
    await contarPendientes(db);
  }
}
