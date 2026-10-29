import type { SQLiteDatabase } from 'expo-sqlite';
import Storage from 'expo-sqlite/kv-store';
import { create } from 'zustand';

import { configServidor, cuerpoEventos, enviarEventos } from '@/telemetria/servidor';

/*
 * Cola de eventos para el servidor de ACPI en SQLite (tabla eventos_pendientes, ver
 * repositorio.ts). Todo evento pasa por aquí: si no hay red, el servidor está dormido o
 * falta la URL, espera en el
 * teléfono y se envía después. La telemetría nunca debe interrumpir los avisos, así
 * que ningún error sale de este módulo.
 */

export type TipoEvento = 'cruce' | 'semaforo' | 'prueba';

const VERSION_FORMATO = 3; // 1: app Kotlin; 3: el cruce suma las métricas de los enlaces
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
  configurada: configServidor() !== null,
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
  const fila = await db.getFirstAsync<{ n: number }>('SELECT COUNT(*) AS n FROM eventos_pendientes');
  useEstadoTelemetria.setState({ pendientes: fila?.n ?? 0 });
}

export async function encolar(db: SQLiteDatabase, tipo: TipoEvento, datos: Record<string, unknown>) {
  if (!useEstadoTelemetria.getState().activa) {
    return;
  }
  // El id deja al servidor descartar duplicados: si se pierde la respuesta, el lote se reenvía
  const evento = JSON.stringify({
    v: VERSION_FORMATO,
    id: idAleatorio(),
    tipo,
    dispositivo: dispositivo(),
    momento: Date.now(),
    ...datos,
  });
  try {
    await db.runAsync('INSERT INTO eventos_pendientes (evento) VALUES (?)', evento);
    await db.runAsync(
      `DELETE FROM eventos_pendientes
       WHERE id NOT IN (SELECT id FROM eventos_pendientes ORDER BY id DESC LIMIT ?)`,
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
  evento: string;
}

let enviando = false;

/** Manda al servidor todo lo pendiente, en lotes; lo que no se pudo enviar queda en la cola */
export async function vaciarCola(db: SQLiteDatabase) {
  const config = configServidor();
  if (enviando || !config) {
    return;
  }
  enviando = true;
  try {
    for (;;) {
      const filas = await db.getAllAsync<FilaPendiente>(
        'SELECT id, evento FROM eventos_pendientes ORDER BY id LIMIT ?',
        LOTE,
      );
      if (filas.length === 0) {
        break;
      }
      const { resultado, detalle } = await enviarEventos(config, cuerpoEventos(filas.map((f) => f.evento)));
      const hora = new Date().toLocaleTimeString();
      useEstadoTelemetria.setState({
        ultimoEnvio: `${hora}  ${resultado === 'enviado' ? `${filas.length} eventos enviados` : detalle}`,
      });
      if (resultado === 'reintentar') {
        break;
      }
      await db.runAsync(
        `DELETE FROM eventos_pendientes WHERE id IN (${filas.map(() => '?').join(',')})`,
        ...filas.map((f) => f.id),
      );
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
    await db.runAsync('DELETE FROM eventos_pendientes');
    await contarPendientes(db);
  }
}
