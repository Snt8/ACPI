import type { SQLiteDatabase } from 'expo-sqlite';

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
];

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

export async function crearSemaforo(db: SQLiteDatabase, datos: DatosSemaforo): Promise<number> {
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
}

export async function eliminarSemaforo(db: SQLiteDatabase, id: number) {
  await db.runAsync('DELETE FROM semaforos WHERE id = ?', id);
}
