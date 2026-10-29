import type { SQLiteDatabase } from 'expo-sqlite';

import { encolar } from '@/telemetria/cola';

export interface Coordenadas {
  latitud: number;
  longitud: number;
}

export interface Semaforo extends Coordenadas {
  id: number;
  nombre: string;
  notas: string;
  precisionMetros: number | null; // del GPS al guardarlo; null si se marcó en el mapa
  creadoEn: number; // ms desde epoch
}

export type DatosSemaforo = Omit<Semaforo, 'id' | 'creadoEn'>;

interface Fila {
  id: number;
  nombre: string;
  notas: string;
  latitud: number;
  longitud: number;
  precision_metros: number | null;
  creado_en: number;
}

const aSemaforo = (f: Fila): Semaforo => ({
  id: f.id,
  nombre: f.nombre,
  notas: f.notas,
  latitud: f.latitud,
  longitud: f.longitud,
  precisionMetros: f.precision_metros,
  creadoEn: f.creado_en,
});

// Cada entrada lleva la base de la versión i a la i + 1 (PRAGMA user_version)
const MIGRACIONES = [
  `CREATE TABLE semaforos (
     id INTEGER PRIMARY KEY AUTOINCREMENT,
     nombre TEXT NOT NULL,
     notas TEXT NOT NULL DEFAULT '',
     latitud REAL NOT NULL,
     longitud REAL NOT NULL,
     precision_metros REAL,
     creado_en INTEGER NOT NULL
   );`,
  // Eventos que esperan su envío a Loki (telemetria/cola.ts)
  `CREATE TABLE envios_pendientes (
     id INTEGER PRIMARY KEY AUTOINCREMENT,
     tipo TEXT NOT NULL,
     momento_ns TEXT NOT NULL,
     linea TEXT NOT NULL
   );`,
];

// Origen de un semáforo: guardado por la persona o detectado por el brazalete en un cruce
export type OrigenSemaforo = 'manual' | 'brazalete';

/** Cada cambio también va a Loki, para mostrar los semáforos en la web de ACPI */
function reportar(db: SQLiteDatabase, accion: 'creado' | 'editado' | 'borrado', id: number, datos?: DatosSemaforo, origen?: OrigenSemaforo) {
  return encolar(db, 'semaforo', {
    accion,
    semaforo_id: id,
    ...(datos && {
      nombre: datos.nombre,
      lat: datos.latitud,
      lon: datos.longitud,
      precision_m: datos.precisionMetros === null ? null : Math.round(datos.precisionMetros),
    }),
    ...(origen && { origen }),
  });
}

/** onInit del SQLiteProvider: crea o actualiza las tablas */
export async function migrarBase(db: SQLiteDatabase) {
  await db.execAsync('PRAGMA journal_mode = WAL;');
  const fila = await db.getFirstAsync<{ user_version: number }>('PRAGMA user_version');
  let version = fila?.user_version ?? 0;
  while (version < MIGRACIONES.length) {
    await db.withTransactionAsync(async () => {
      await db.execAsync(MIGRACIONES[version]);
      await db.execAsync(`PRAGMA user_version = ${version + 1}`);
    });
    version++;
  }
}

export async function listarSemaforos(db: SQLiteDatabase): Promise<Semaforo[]> {
  const filas = await db.getAllAsync<Fila>('SELECT * FROM semaforos ORDER BY nombre COLLATE NOCASE');
  return filas.map(aSemaforo);
}

export async function obtenerSemaforo(db: SQLiteDatabase, id: number): Promise<Semaforo | null> {
  const fila = await db.getFirstAsync<Fila>('SELECT * FROM semaforos WHERE id = ?', id);
  return fila ? aSemaforo(fila) : null;
}

export async function crearSemaforo(
  db: SQLiteDatabase,
  datos: DatosSemaforo,
  origen: OrigenSemaforo = 'manual',
): Promise<number> {
  const resultado = await db.runAsync(
    `INSERT INTO semaforos (nombre, notas, latitud, longitud, precision_metros, creado_en)
     VALUES (?, ?, ?, ?, ?, ?)`,
    datos.nombre,
    datos.notas,
    datos.latitud,
    datos.longitud,
    datos.precisionMetros,
    Date.now(),
  );
  await reportar(db, 'creado', resultado.lastInsertRowId, datos, origen);
  return resultado.lastInsertRowId;
}

export async function actualizarSemaforo(db: SQLiteDatabase, id: number, datos: DatosSemaforo) {
  await db.runAsync(
    `UPDATE semaforos
     SET nombre = ?, notas = ?, latitud = ?, longitud = ?, precision_metros = ?
     WHERE id = ?`,
    datos.nombre,
    datos.notas,
    datos.latitud,
    datos.longitud,
    datos.precisionMetros,
    id,
  );
  await reportar(db, 'editado', id, datos);
}

export async function eliminarSemaforo(db: SQLiteDatabase, id: number) {
  await db.runAsync('DELETE FROM semaforos WHERE id = ?', id);
  await reportar(db, 'borrado', id);
}
